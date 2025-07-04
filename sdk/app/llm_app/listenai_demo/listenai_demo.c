#include "listenai_demo.h"
#include "sounds.h"

/*
* 功能说明：
*
* 按键方案：
*    短按后设备进行应答（“我在”）后开始监听，此时进行语音输入，
*    vad 检测静音后停止监听，思考后回复对话，
*    回复结束后自动重新开始监听，可继续语音输入，重复此流程。
*    再次短按可打断重新开始。静音一段时间，提示（“那我先退下了”）。
* 唤醒词方案：
*    唤醒后设备进行应答（“我在”）后开始监听，此时进行语音输入，
*    vad 检测静音后停止监听，思考后回复对话，
*    回复结束后自动重新开始监听，可继续语音输入，重复此流程。
*    再次唤醒可打断重新开始。静音一段时间，提示（“那我先退下了”）。
*/
#ifdef LISTENAI_DEMO

#define LISTENAI_URL        "https://api.listenai.com/v1/auth/tokens"

#define LISTENAI_GATEWAY_URL    "wss://api.listenai.com/v1/interaction"

#define PRODUCT_ID  "xxx"
#define SECRET_ID   "xxx"
#define DEVICE_ID   "xxx"

struct os_task listenai_task_demo;
struct listenai_demo_manage listenai_mgr = {
    .key_transfer       = 0,
    .auadc_model_start  = 0,
    .connected          = 0,
    .finish             = 0,
    .timeout            = 0,
};

int32 listenai_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case LLM_EVENT_CONNECTED:
            os_printf("listenai connected!\r\n");
            listenai_mgr.connected = 1;
            listenai_mgr.connect_audio_play = 1;
            break;
        case LLM_EVENT_DISCONNECT:
            os_printf("listenai disconnect!\r\n");
            listenai_mgr.connected = 0;
            listenai_mgr.disconnect_audio_play = 1;
            break;
        case LLM_EVENT_STT_RESULT:
            os_printf("STT(%d):", os_strlen((char *)param1));
            hgprintf_out((char *)param1, os_strlen((char *)param1), 0);
            _os_printf("\r\n");
            listenai_mgr.timeout |= 0x2;
            break;
        case LLM_EVENT_TTS_RESULT:
            os_printf("TTS(%d):", os_strlen((char *)param1));
            hgprintf_out((char *)param1, os_strlen((char *)param1), 0);
            _os_printf("\r\n");
            break;
        case LLM_EVENT_VAD_RESULT:
            os_printf("listenai check vad!\r\n");
            listenai_mgr.auadc_model_start = 0;
            listenai_mgr.timeout |= 0x1;
            break;
        case LLM_EVENT_FINISH_RESULT:
            os_printf("listenai check finish!\r\n");
            listenai_mgr.finish = 1;
            break;
        case LLM_EVENT_ERROR_MSG:
            os_printf("ERR_MSG:%s\r\n", (char *)param1);
            break;
        case LLM_EVENT_TIMEOUT:
            os_printf("TIMEOUT!\r\n");
            break;
        default:
            break;
    }
    return RET_OK;
}

const struct llm_model models[] = {
    {"listenai_aiui", &listenai_aiui_model},
};

struct llm_global_param global = {
    .task_reuse     = 0,
    .model_count    = 1,
    .models         = models,
};

struct llm_sts_sparam sts_session_cfg = {
    .audio_url_cnt          = 8,
    .qmsg_tx_cnt            = 16,
    .rx_buff_size           = 2048,
    .cb_max_size            = 1024,
    .retry_cnt              = 2,
    .evt_cb                 = listenai_event_cb,
};
struct Listenai_AIUI_Cfg aiui_sts_cfg = {
    .token_url = LISTENAI_URL,
    .gateway_url = LISTENAI_GATEWAY_URL,
    .product_id = PRODUCT_ID,
    .secret_id = SECRET_ID,
    .device_id = DEVICE_ID,

    .data_type = "audio",
    .aue = NULL,
    .features = {"nlu", "tts", NULL},

    .asr_evad = "0",
    .asr_vad_eos = "800",
    .asr_ent = "home-va",
    .asr_svad = NULL,

    .tts_vcn = "x4_lingxiaoqi_oral",
    .tts_ent = NULL,
    .tts_volume = "100",
    .tts_speed = "60",
    .tts_pitch = "50",

    .nlu_nlp_mode = NULL,
    .nlu_scene = NULL,
    .nlu_clean_dialog_history = NULL,
};
struct llm_trans_param sts_trans_cfg = {
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL,
};

int32 listenai_mp3_read_func(void *ptr, uint32_t size)
{
    int32 read_len = 0;
    while (read_len <= 0) {
        read_len = llm_sts_recv(listenai_mgr.sts_session, ptr, size);
        os_sleep_ms(10);
    }
    return read_len;
}

static int32 listenai_opcode_func(stream *s, void *priv, int opcode)
{
    int32 res = 0;
    switch (opcode) {
        case STREAM_OPEN_ENTER:
            break;
        case STREAM_OPEN_EXIT: {
            enable_stream(s, 1);
        }
        break;
        case STREAM_OPEN_FAIL:
            break;
        default:
            break;
    }
    return res;
}

int32 listenai_app_init(char *llm_name)
{
    int32 result = RET_OK;
    listenai_mgr.sts_session  = llm_sts_init(llm_name, &sts_session_cfg);
    if (!listenai_mgr.sts_session) {
        os_printf("chat session init fail!\r\n");
        return RET_ERR;
    }
    result = llm_sts_config(listenai_mgr.sts_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&sts_trans_cfg, sizeof(sts_trans_cfg));
    if (result) {
        os_printf("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    result = llm_sts_config(listenai_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&aiui_sts_cfg, sizeof(aiui_sts_cfg));
    if (result) {
        os_printf("sts_cfg fail!\r\n");
        return RET_ERR;
    }
    return result;
}

#ifdef LLM_SPV12XX
struct os_task listenai_task_awaken;

#define DEBOUNCE_COUNT  3       // 去抖动计数器阈值，连续检测到相同状态3次认为有效
#define COOLDOWN_TIME   3000    // 冷却时间，单位为毫秒（3秒）

void listenai_awaken_demo(void)
{
    // 设置 LLM_WK_IO 为输入模式，并配置上拉电阻为 100K 欧姆
    gpio_set_dir(LLM_WK_IO, GPIO_DIR_INPUT);
    gpio_set_mode(LLM_WK_IO, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

    uint64 low_start_time = 0;      // 记录引脚拉低开始的时间（单位：毫秒）
    uint64 cooldown_end_time = 0;   // 记录冷却结束的时间（单位：毫秒）
    bool is_low = false;            // 标记是否处于低电平状态
    int debounce_counter = 0;       // 去抖动计数器
    bool last_state = true;         // 记录上一次读取的状态，默认高电平

    while (1) {
        os_sleep_ms(1);
        uint64 current_time = os_jiffies_to_msecs(os_jiffies());
        // 如果处于冷却时间内，跳过检测逻辑
        if (current_time < cooldown_end_time) {
            continue;
        }

        bool current_state = gpio_get_val(LLM_WK_IO) == 0; // 当前状态，true 表示低电平
        if (current_state == last_state) {
            // 如果当前状态与上次相同，增加计数器
            debounce_counter++;
            if (debounce_counter >= DEBOUNCE_COUNT) {
                debounce_counter = DEBOUNCE_COUNT; // 防止计数器溢出

                // 如果低电平刚刚被确认有效，则记录时间为当前时间减去 DEBOUNCE_COUNT 消耗的时间
                if (!is_low && current_state) { // 从高变低
                    is_low = true;
                    low_start_time = current_time - DEBOUNCE_COUNT; // 减去去抖动消耗的时间
                }

                // 如果已经是低电平状态，检查持续时间
                if (is_low) {
                    uint64 duration = current_time - low_start_time;
                    //os_printf(KERN_ERR"LLM_WK_IO pulled low for %ums.\r\n", duration);

                    // 判断是否在 16ms 到 20ms 范围内
                    if (duration >= 16 && duration <= 20) {
                        // 持续低电平在 16ms 到 20ms 范围内，触发唤醒逻辑
                        os_printf(KERN_ERR"Wakeup triggered!\r\n");

                        is_low = false;         // 重置低电平状态
                        debounce_counter = 0;   // 重置去抖动计数器
                        last_state = true;      // 重置上次状态

                        audac_disable_play();
                        // 设置冷却结束时间
                        cooldown_end_time = current_time + COOLDOWN_TIME;

                        listenai_mgr.key_transfer = 1;
                        continue;
                    }
                }
            }
        } else {
            debounce_counter = 0;
        }
        last_state = current_state;
    }
}
#else
uint32_t listenai_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    if ((keyvalue >> 8) != AD_PRESS)
    { return 0; }
    uint32 key_val = (keyvalue & 0xff);
    if (key_val == KEY_EVENT_DOWN) {
        os_printf(KERN_ERR"Wakeup triggered!\r\n");
        listenai_mgr.key_transfer = 1;
    }
    return 0;
}
#endif

void listenai_main_demo(void)
{
    int32 result = RET_OK;
    struct data_structure *data_s = NULL;
    char *data = NULL;
    uint32 data_len = 0;

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");
    
    os_printf("Waiting to obtain network time...");
    while (!sys_status.ntp_update) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Successfully obtained network time!\r\n");
    
    gethostbyname_async("api.iflyos.cn");
    gethostbyname_async("api.listenai.com");

    llm_global_init(&global);
    result = listenai_app_init("listenai_aiui");
    if (result) {
        goto cleanup;
    }

    //microphone
    stream *s = open_stream_available(R_SPEECH_RECOGNITION, 0, 8, listenai_opcode_func, NULL);
    if (!s) {
        os_printf("open speech_recognition stream err!\r\n");
        goto cleanup;
    }

    //Player init
    mp3_decode_init(NULL, listenai_mp3_read_func);
    audac_enable_play();

#ifndef LLM_SPV12XX
    add_keycallback(listenai_intercom_push_key, NULL);
#else
    OS_TASK_INIT("LISTENAI_AWAKEN", &listenai_task_awaken, listenai_awaken_demo, NULL, OS_TASK_PRIORITY_NORMAL + 1, 1024);
#endif

    while (1) {
startup:
        if (listenai_mgr.connected == 1) {
            if (listenai_mgr.connect_audio_play == 1) {
                audac_disable_play();
                mp3_decode_clean_stream();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                llm_sts_play_audio(listenai_mgr.sts_session, yilianjie, yilianjie_size);
                listenai_mgr.connect_audio_play = 0;
            }
            if (listenai_mgr.key_transfer) {
                llm_sts_stop(listenai_mgr.sts_session);
                audac_disable_play();
                mp3_decode_clean_stream();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                do {
                    result = llm_sts_wait(listenai_mgr.sts_session, 500);
                    if (listenai_mgr.connected == 0) {
                        os_printf("This can be confirmed to be a disconnection!\r\n");
                        goto startup;
                    }
                    if (result != RET_OK && result != LLME_WAIT_TIMEOUT) {
                        os_printf("Maybe it's disconnected?\r\n");
                        goto startup;
                    }
                } while (result == LLME_WAIT_TIMEOUT);
                llm_sts_play_audio(listenai_mgr.sts_session, wozai, wozai_size);
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                os_printf("请说话\r\n");
                listenai_mgr.auadc_model_start = 1;
                listenai_mgr.finish = 0;
                listenai_mgr.timeout = 0;
                listenai_mgr.key_transfer = 0;
            }

            if (listenai_mgr.finish) {
                if (listenai_mgr.timeout != 0x3) {
                    llm_sts_stop(listenai_mgr.sts_session);
                    audac_disable_play();
                    mp3_decode_clean_stream();
                    while (!audac_wait_empty()) {
                        os_sleep_ms(1);
                    }
                    audac_enable_play();
                    llm_sts_play_audio(listenai_mgr.sts_session, tuixia, tuixia_size);
                    while (!audac_wait_empty()) {
                        os_sleep_ms(1);
                    }
                    listenai_mgr.auadc_model_start = 0;
                    listenai_mgr.finish = 0;
                    listenai_mgr.timeout = 0;
                } else {
                    if (llm_sts_check_audio(listenai_mgr.sts_session)) {
                       do {
                            result = llm_sts_wait(listenai_mgr.sts_session, 500);
                            if (listenai_mgr.connected == 0) {
                                os_printf("This can be confirmed to be a disconnection!\r\n");
                                goto startup;
                            }
                            if (result != RET_OK && result != LLME_WAIT_TIMEOUT) {
                                os_printf("Maybe it's disconnected?\r\n");
                                goto startup;
                            }
                        } while (result == LLME_WAIT_TIMEOUT);
                        os_printf("new dialogue.\r\n");
                        os_printf("请说话\r\n");
                        listenai_mgr.auadc_model_start = 1;
                        listenai_mgr.finish = 0;
                        listenai_mgr.timeout = 0;
                    }
                }
            }
            
            data_s = recv_real_data(s);
            if (data_s) {
                data = get_stream_real_data(data_s);
                data_len = get_stream_real_data_len(data_s);
                if (listenai_mgr.auadc_model_start) {
                    if (data) {
                        llm_sts_send(listenai_mgr.sts_session, data, data_len, LLM_TRANSFERRING);
                        //os_printf("send audio len:%d\r\n", data_len);
                    }
                }
                free_data(data_s);
                data_s = NULL;
            }
        } else {
            if (listenai_mgr.disconnect_audio_play == 1) {
                audac_disable_play();
                mp3_decode_clean_stream();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                llm_sts_play_audio(listenai_mgr.sts_session, yiduankai, yiduankai_size);
                listenai_mgr.disconnect_audio_play = 0;
            }

            if (listenai_mgr.key_transfer == 1) {
                os_printf("网络异常，等待重新连接！\r\n");
                listenai_mgr.key_transfer = 0;
            }
            if (listenai_mgr.auadc_model_start == 1) {
                listenai_mgr.auadc_model_start = 0;
            }
            if (listenai_mgr.finish == 1) {
                listenai_mgr.finish = 0;
            }
        }
        os_sleep_ms(10);
    }

cleanup:
    llm_sts_deinit(listenai_mgr.sts_session);
    llm_global_deinit();
}

void listenai_demo(void)
{
    OS_TASK_INIT("LISTENAI_DEMO", &listenai_task_demo, listenai_main_demo, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
}

#endif

