#include "voice_conversation_demo.h"
#include "sounds.h"

/*
* 功能说明：
*
* 按键方案：
*    长按设备开始监听，此时进行语音输入，
*    松开设备停止监听，思考后回复对话。
* 唤醒词方案：
*    唤醒后设备进行应答（“我在”）后开始监听，此时进行语音输入，
*    vad 检测静音后停止监听，思考后回复对话。
*/
#ifdef LLM_DEMO

/* Please add your server information */
#define CHAT_URL            "https://ark.cn-beijing.volces.com/api/v3/chat/completions"
#define CONTEXT_URL         "https://ark.cn-beijing.volces.com/api/v3/context/create"
#define CONTEXT_CHAT_URL    "https://ark.cn-beijing.volces.com/api/v3/context/chat/completions"
#define ARK_API_KEY         "xxx"
#define ENDPOINT_ID         "xxx"

#define STT_URL             "wss://openspeech.bytedance.com/api/v2/asr"
#define STT_APPID           "xxx"
#define STT_TOKEN           "xxx"
#define STT_CLUSTER         "xxx"

#define TTS_URL             "wss://openspeech.bytedance.com/api/v1/tts/ws_binary"
#define TTS_APPID           "xxx"
#define TTS_TOKEN           "xxx"
#define TTS_CLUSTER         "xxx"

struct llm_demo_manage user_mgr = {
    .key_transfer       = 0,
    .auadc_model_start  = 0,
    .key_transfer_old   = 0,
    .error              = 0,
    .stt_rx_len         = 0,
    .chat_rx_len        = 0,
};

void llm_app_clean(void)
{
    char *chat_resp = NULL;

    os_printf("clean start\r\n");
    
    //清除上次的资源
    llm_chat_stop(user_mgr.chat_session);
    llm_stt_stop(user_mgr.stt_session);
    llm_tts_stop(user_mgr.tts_session);
    os_printf("clean done\r\n");
    
    os_memset(user_mgr.stt_recv, 0, STT_RECV_SIZE);
    os_memset(user_mgr.chat_recv, 0, CHAT_RECV_SIZE);
    
    mp3_decode_clean_stream();
    
    do {
        if (chat_resp) { os_free(chat_resp); }
    } while (RB_INT_GET(&user_mgr.chat_resp, chat_resp));
    RB_RESET(&user_mgr.chat_resp);
    chat_resp = NULL;
}

int32 llm_app_chat_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case LLM_EVENT_CHAT_CONTEXT_ID:
            os_printf("CHAT content_id: %s\n", (char *)param1);
            break;
        case LLM_EVENT_ERROR_MSG:
            os_printf("CHAT ERR MSG: %s\r\n", (char *)param1);
            break;
        case LLM_EVENT_CHAT_RESULT:
            if (param1 != RET_OK) {
                os_printf("CHAT failed!!(%d)\r\n", param1);
                user_mgr.error = 1;
            }
            break;
        default:
            break;
    }
    return RET_OK;
}

int32 llm_app_tts_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    char *chat_resp = NULL;

    switch (evt) {
        case LLM_EVENT_TTS_AUDIO_START:
            if (RB_INT_GET(&user_mgr.chat_resp, chat_resp)) {
                os_printf("chat text(%d):", os_strlen(chat_resp));
                hgprintf_out(chat_resp, os_strlen(chat_resp), 0);
                _os_printf("\r\n");
                os_free(chat_resp);
            }
            break;
        case LLM_EVENT_TTS_AUDIO_ERR:
            os_printf("TTS AUDIO ERR\r\n");
            if (RB_INT_GET(&user_mgr.chat_resp, chat_resp)) {
                os_free(chat_resp);
            }
            break;
        case LLM_EVENT_ERROR_MSG:
            os_printf("TTS ERR MSG: %s\r\n", (char *)param1);
            break;
        case LLM_EVENT_TTS_RESULT:
            if (param1 != RET_OK) {
                if (param1 == LLME_INTR) {
                    os_printf("TTS interrupt(%d)\r\n", param1);
                } else {
                    os_printf("TTS failed!!(%d)\r\n", param1);
                    user_mgr.error = 1;
                }
            }
            break;
        case LLM_EVENT_TIMEOUT:
            os_printf("TTS No data sent within 5s, automatically disconnected!\r\n");
            break;
        default:
            break;
    }
    return RET_OK;
}

int32 llm_app_stt_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case LLM_EVENT_ERROR_MSG:
            os_printf("STT ERR MSG: %s\r\n", (char *)param1);
            break;
        case LLM_EVENT_STT_RESULT:
            if (param1 != RET_OK) {
                if (param1 == LLME_INTR) {
                    os_printf("STT interrupt(%d)\r\n", param1);
                } else {
                    os_printf("STT failed!!(%d)\r\n", param1);
                    user_mgr.error = 1;
                }
            }
            break;
        case LLM_EVENT_TIMEOUT:
            os_printf("STT No data sent within 5s, automatically disconnected!\r\n");
            break;
        default:
            break;
    }
    return RET_OK;
}

////////////////////////////////////////////
const struct llm_model models[] = {
    {"doubao_llm_chat", &doubao_llm_chat},
    {"doubao_asr_stt",  &doubao_asr_stt},
    {"doubao_asr_tts",  &doubao_asr_tts},
};

struct llm_global_param global = {
    .task_reuse     = 1,
    .model_count    = 3,
    .models         = models,
};

//CHAT
struct llm_chat_sparam chat_session_cfg = {
    .qmsg_tx_cnt            = 4,
    .qmsg_rx_cnt            = 16,
    .reply_fragment_size    = 3 * 50 + 10,
    .context_id_len         = 24,
    .evt_cb                 = llm_app_chat_event_cb,
};
struct Doubao_CHAT_Cfg chat_platform_cfg = {
    .chat_url               = CHAT_URL,
    .ark_api_key            = ARK_API_KEY,
    .endpoint_id            = ENDPOINT_ID,
    .stop                   = NULL,
    .max_tokens             = "4096",
    .context_url            = CONTEXT_URL,
    .context_chat_url       = CONTEXT_CHAT_URL,
    .ttl                    = "3600",
    .last_history_tokens    = "4096",
};
struct llm_trans_param chat_trans_cfg = {
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 3,
    .blob_len           = 0,
    .blob_data          = NULL,
};

//TTS
struct llm_tts_sparam tts_session_cfg = {
    .qmsg_tx_cnt            = 32,
    .rx_buff_size           = 2048,
    .cb_max_size            = 1024,
    .evt_cb                 = llm_app_tts_event_cb,
};
struct Doubao_ASR_TTS_Cfg asr_tts_cfg = {
    .url                    = TTS_URL,
    .app_appid              = TTS_APPID,
    .app_token              = TTS_TOKEN,
    .app_cluster            = TTS_CLUSTER,
    .user_uid               = "41494922",
    .audio_voice_type       = "BV700_streaming",
    .audio_rate             = "8000",
    .audio_compression_rate = NULL,
    .audio_encoding         = "mp3",
    .audio_speed_ratio      = "1.0",
    .audio_volume_ratio     = "0.5",
    .audio_pitch_ratio      = "1.0",
    .audio_emotion          = "comfort",
    .audio_language         = "cn",
    .silence_duration       = NULL,
};
struct llm_trans_param tts_trans_cfg = {
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 3,
    .blob_len           = 0,
    .blob_data          = NULL,
};

//STT
struct llm_stt_sparam stt_session_cfg = {
    .qmsg_tx_cnt            = 32,
    .qmsg_rx_cnt            = 4,
    .cb_max_size            = 1024,
    .evt_cb                 = llm_app_stt_event_cb,
};
struct Doubao_ASR_STT_Cfg asr_stt_cfg = {
    .url = STT_URL,
    .app_appid = STT_APPID,
    .app_token = STT_TOKEN,
    .app_cluster = STT_CLUSTER,
    .user_uid = "41494922",
    .audio_format = "raw",
    .audio_codec = "raw",
    .audio_rate = "16000",
    .audio_bits = "16",
    .audio_channel = "1",
    .boosting_table_name = NULL,
    .correct_table_name = NULL,
};
struct llm_trans_param stt_trans_cfg = {
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 3,
    .blob_len           = 0,
    .blob_data          = NULL,
};
////////////////////////////////////////////

static int32 opcode_func(stream *s, void *priv, int opcode)
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

int32 llm_mp3_read_func(void *ptr, uint32_t size)
{
    int32 read_len = 0;
    while (read_len <= 0) {
        read_len = llm_tts_recv(user_mgr.tts_session, ptr, size);
        os_sleep_ms(10);
    }
    return read_len;
}

int32 llm_chat_app_init(char *llm_name)
{
    int32 result = RET_OK;
    user_mgr.chat_session  = llm_chat_init(llm_name, &chat_session_cfg);
    if (!user_mgr.chat_session) {
        os_printf("chat session init fail!\r\n");
        return RET_ERR;
    }
    result = llm_chat_config(user_mgr.chat_session, LLM_CONFIG_TYPE_TRANS,
                             (void *)&chat_trans_cfg, sizeof(chat_trans_cfg));
    if (result) {
        os_printf("chat_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    result = llm_chat_config(user_mgr.chat_session, LLM_CONFIG_TYPE_MODEL,
                             (void *)&chat_platform_cfg, sizeof(chat_platform_cfg));
    if (result) {
        os_printf("chat_cfg fail!\r\n");
        return RET_ERR;
    }
    return result;
}

int32 llm_stt_app_init(char *llm_name)
{
    int32 result = RET_OK;

    user_mgr.stt_session   = llm_stt_init(llm_name, &stt_session_cfg);
    if (!user_mgr.stt_session) {
        os_printf("stt session init fail!\r\n");
        return RET_ERR;
    }
    result = llm_stt_config(user_mgr.stt_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&stt_trans_cfg, sizeof(stt_trans_cfg));
    if (result) {
        os_printf("stt_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    result = llm_stt_config(user_mgr.stt_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&asr_stt_cfg, sizeof(asr_stt_cfg));
    if (result) {
        os_printf("asr_stt_cfg fail!\r\n");
        return RET_ERR;
    }
    return result;
}

int32 llm_tts_app_init(char *llm_name)
{
    int32 result = RET_OK;

    user_mgr.tts_session   = llm_tts_init(llm_name, &tts_session_cfg);
    if (!user_mgr.tts_session) {
        os_printf("tts session init fail!\r\n");
        return RET_ERR;
    }
    result = llm_tts_config(user_mgr.tts_session, LLM_CONFIG_TYPE_TRANS,
                            (void *)&tts_trans_cfg, sizeof(tts_trans_cfg));
    if (result) {
        os_printf("tts_trans_cfg fail!\r\n");
        return RET_ERR;
    }
    result = llm_tts_config(user_mgr.tts_session, LLM_CONFIG_TYPE_MODEL,
                            (void *)&asr_tts_cfg, sizeof(asr_tts_cfg));
    if (result) {
        os_printf("asr_tts_cfg fail!\r\n");
        return RET_ERR;
    }
    return result;
}

#ifdef LLM_SPV12XX
struct os_task llm_task_awaken;

#define DEBOUNCE_COUNT  3       // 去抖动计数器阈值，连续检测到相同状态3次认为有效
#define COOLDOWN_TIME   1500    // 冷却时间，单位为毫秒（1.5秒）

void llm_awaken_demo(void)
{
    char *chat_resp = NULL;

    // 设置 LLM_WK_IO 为输入模式，并配置上拉电阻为 100K 欧姆
    gpio_set_dir(LLM_WK_IO, GPIO_DIR_INPUT);
    gpio_set_mode(LLM_WK_IO, GPIO_PULL_UP, GPIO_PULL_LEVEL_100K);

    uint64 low_start_time = 0; // 记录引脚拉低开始的时间（单位：毫秒）
    uint64 cooldown_end_time = 0;    // 记录冷却结束的时间（单位：毫秒）
    bool is_low = false;         // 标记是否处于低电平状态
    int debounce_counter = 0;     // 去抖动计数器
    bool last_state = true;       // 记录上一次读取的状态，默认高电平

    while (1) {
        os_sleep_ms(1);
        uint64 current_time = os_jiffies_to_msecs(os_jiffies());
        // 如果处于冷却时间内，跳过检测逻辑
        if (current_time < cooldown_end_time) {
            continue;
        }

        if (user_mgr.key_transfer == 1 && auadc_get_vad_res() == 0) {
            os_mutex_lock(&user_mgr.lock, osWaitForever);
            os_printf("key stop\r\n");
            user_mgr.key_transfer = 0;
            os_mutex_unlock(&user_mgr.lock);
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

                        is_low = false; // 重置低电平状态
                        debounce_counter = 0; // 重置去抖动计数器
                        last_state = true; // 重置上次状态

                        // 设置冷却结束时间
                        cooldown_end_time = current_time + COOLDOWN_TIME;
                        
                        os_mutex_lock(&user_mgr.lock, osWaitForever);
                        os_printf("key start\r\n");
                        user_mgr.key_transfer = 1;
                        os_mutex_unlock(&user_mgr.lock);
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
uint32_t llm_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    if ((keyvalue >> 8) != AD_PRESS)
    { return 0; }
    uint32 key_val = (keyvalue & 0xff);
    if ((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
        if (user_mgr.key_transfer == 0) {
            os_mutex_lock(&user_mgr.lock, osWaitForever);
            os_printf("key start\r\n");
            user_mgr.key_transfer = 1;
            os_mutex_unlock(&user_mgr.lock);
        }
    } else if ((key_val == KEY_EVENT_LUP)) {
        os_mutex_lock(&user_mgr.lock, osWaitForever);
        os_printf("key stop\r\n");
        user_mgr.key_transfer = 0;
        os_mutex_unlock(&user_mgr.lock);
    }
    return 0;
}
#endif

void llm_main_demo(void)
{
    int32 result = RET_OK;

    int32 tx_result     = 0;
    int32 tts_temp      = 1;
    char **chat_resp_buff = NULL;
    char *chat_recv_copy = NULL;    

    stream *s = NULL;
    struct data_structure *data_s = NULL;
    char *data = NULL;
    uint32 data_len = 0;
    uint8 last_sta = 0;

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    gethostbyname_async("ark.cn-beijing.volces.com");
    gethostbyname_async("openspeech.bytedance.com");

    os_mutex_init(&user_mgr.lock);

    //microphone
    s = open_stream_available(R_SPEECH_RECOGNITION, 0, 8, opcode_func, NULL);
    if (!s) {
        os_printf("open speech_recognition stream err!\r\n");
        return;
    }
    //Player init
    mp3_decode_init(NULL, llm_mp3_read_func);
    audac_enable_play();
#ifndef LLM_SPV12XX
    //Key init
    add_keycallback(llm_intercom_push_key, NULL);
#endif

    //user rx buff
    user_mgr.stt_recv = os_malloc(STT_RECV_SIZE);
    user_mgr.chat_recv = os_malloc(CHAT_RECV_SIZE);
    if (!user_mgr.stt_recv || !user_mgr.chat_recv) {
        os_printf("user rx buff malloc fail!\r\n");
        goto cleanup;
    }

    chat_resp_buff = os_malloc((32 + 1) * sizeof(char *));
    if (!chat_resp_buff) {
        os_printf("no memory!\r\n");
        goto cleanup;
    }
    RB_INIT_R(&user_mgr.chat_resp, 32, chat_resp_buff);

    //llm init
    llm_global_init(&global);
    result = llm_chat_app_init("doubao_llm_chat");
    if (result) {
        goto cleanup;
    }
    result = llm_stt_app_init("doubao_asr_stt");
    if (result) {
        goto cleanup;
    }
    result = llm_tts_app_init("doubao_asr_tts");
    if (result) {
        goto cleanup;
    }

    llm_chat_set_role(user_mgr.chat_session, "你是一个乐于助人的智能助手");
    do {
        result = llm_chat_new_dialogue(user_mgr.chat_session, CONTEXT_MODE_PREFIX);
        os_sleep_ms(10);
    } while (result);

#ifdef LLM_SPV12XX
    char *copy_buf = NULL;
    uint32 copy_offset = 0;

    OS_TASK_INIT("LLM_AWAKEN", &llm_task_awaken, llm_awaken_demo, NULL, OS_TASK_PRIORITY_NORMAL + 1, 1024);
#endif

    llm_tts_play_audio(user_mgr.tts_session, yilianjie, yilianjie_size);
    os_sleep(1);

    while (1) {
        if (user_mgr.error == 1) {
            audac_disable_play();
            llm_app_clean();
            audac_enable_play();
            user_mgr.key_transfer_old = 0;
            user_mgr.key_transfer = 0;
            user_mgr.auadc_model_start = 0;
            user_mgr.error = 0;
            os_printf("对话异常，退出对话\r\n");
        }
        
        if (user_mgr.key_transfer_old != user_mgr.key_transfer) {
            os_mutex_lock(&user_mgr.lock, osWaitForever);
            os_printf("key change %d --> %d\r\n", user_mgr.key_transfer_old, user_mgr.key_transfer);
            if (user_mgr.key_transfer == 1) {
                audac_disable_play();
                llm_app_clean();

                llm_stt_send(user_mgr.stt_session, NULL, 0, LLM_STATR_TRANSFER);

#ifdef LLM_SPV12XX
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                };
                audac_enable_play();
                llm_tts_play_audio(user_mgr.tts_session, wozai, wozai_size);
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                };
//#else
                //llm_tts_play_audio(user_mgr.tts_session, beep, beep_size);
#endif
                //触发采集
                user_mgr.auadc_model_start = 1;
                os_printf("mic start\r\n");
            } else {
                audac_enable_play();
                user_mgr.auadc_model_start = 0;
                os_printf("mic stop\r\n");
            }
            user_mgr.key_transfer_old = user_mgr.key_transfer;
            os_mutex_unlock(&user_mgr.lock);
        }
        
        data_s = recv_real_data(s);
#ifndef LLM_SPV12XX
        if (data_s) {
            data = get_stream_real_data(data_s);
            data_len = get_stream_real_data_len(data_s);
            if (user_mgr.auadc_model_start) {
                llm_stt_send(user_mgr.stt_session, data, data_len, LLM_TRANSFERRING);
                //os_printf("send audio data(%d)!\r\n", data_len);
            } else if ((last_sta) && (!user_mgr.auadc_model_start)) {
                llm_stt_send(user_mgr.stt_session, data, data_len, LLM_STOP_TRANSFER);
                //os_printf("send end audio data(%d)!\r\n", data_len);
            }
            last_sta = user_mgr.auadc_model_start;
            free_data(data_s);
            data_s = NULL;
        }
#else
        if (data_s) {
            data = get_stream_real_data(data_s);
            data_len = get_stream_real_data_len(data_s);

            if (user_mgr.auadc_model_start) {
                if (!copy_buf) {
                    copy_buf = os_malloc(data_len * 6);  //data_len:320
                }
                os_memcpy(copy_buf + copy_offset, data, data_len);
                copy_offset += data_len;
                if (copy_offset >= data_len * 6) {
                    llm_stt_send(user_mgr.stt_session, copy_buf, copy_offset, LLM_TRANSFERRING);
                    //os_printf(KERN_ERR"send audio len: %d\r\n", copy_offset);
                    os_free(copy_buf);
                    copy_buf = NULL;
                    copy_offset = 0;
                }
            } else if ((last_sta) && (!user_mgr.auadc_model_start)) {
                if (!copy_buf) {
                    copy_buf = os_malloc(data_len * 6);
                }
                os_memcpy(copy_buf + copy_offset, data, data_len);
                copy_offset += data_len;
                llm_stt_send(user_mgr.stt_session, copy_buf, copy_offset, LLM_STOP_TRANSFER);
                //os_printf(KERN_ERR"send last audio len: %d\r\n", copy_offset);
                os_free(copy_buf);
                copy_buf = NULL;
                copy_offset = 0;
            }
            last_sta = user_mgr.auadc_model_start;
            free_data(data_s);
            data_s = NULL;
        } else if ((last_sta) && (!user_mgr.auadc_model_start)) {
            if (copy_buf && copy_offset > 0) {
                llm_stt_send(user_mgr.stt_session, copy_buf, copy_offset, LLM_STOP_TRANSFER);
                //os_printf(KERN_ERR"send last audio len: %d\r\n", copy_offset);
                os_free(copy_buf);
                copy_buf = NULL;
                copy_offset = 0;
            } else {
                llm_stt_send(user_mgr.stt_session, NULL, 0, LLM_STOP_TRANSFER);
                //os_printf(KERN_ERR"send last audio\r\n");
            }
            last_sta = user_mgr.auadc_model_start;
        }
#endif
        user_mgr.stt_rx_len = llm_stt_recv(user_mgr.stt_session, user_mgr.stt_recv, STT_RECV_SIZE);
        if (user_mgr.stt_rx_len > 0) {
            os_printf("stt text(%d):", user_mgr.stt_rx_len);
            hgprintf_out(user_mgr.stt_recv, user_mgr.stt_rx_len, 0);
            _os_printf("\r\n");
            tx_result = llm_chat_send(user_mgr.chat_session, user_mgr.stt_recv, user_mgr.stt_rx_len);
            tx_result = llm_tts_send(user_mgr.tts_session, NULL, 0, LLM_STATR_TRANSFER);
            tts_temp = 0;
        }
        if (!RB_FULL(&user_mgr.chat_resp)) {
            user_mgr.chat_rx_len = llm_chat_recv(user_mgr.chat_session, user_mgr.chat_recv, CHAT_RECV_SIZE);
            if (user_mgr.chat_rx_len > 0) {
                chat_recv_copy = os_strdup(user_mgr.chat_recv);
                if (chat_recv_copy) {
                    RB_INT_SET(&user_mgr.chat_resp, chat_recv_copy);
                }
                if (tts_temp) {
                    tx_result = llm_tts_send(user_mgr.tts_session, NULL, 0, LLM_STATR_TRANSFER);
                }
                do {
                    tx_result = llm_tts_send(user_mgr.tts_session, user_mgr.chat_recv, user_mgr.chat_rx_len, LLM_STOP_TRANSFER);
                    if (user_mgr.error == 1 || user_mgr.key_transfer == 1) {
                        break;
                    }
                } while (tx_result == LLME_AGAIN);
                tts_temp = 1;
            }
        }
        os_sleep_ms(10);
    }

cleanup:
    if (user_mgr.stt_recv) { os_free(user_mgr.stt_recv); }
    if (user_mgr.chat_recv) { os_free(user_mgr.chat_recv); }
    if (chat_resp_buff) { os_free(chat_resp_buff); }
    llm_chat_deinit(user_mgr.chat_session);
    llm_stt_deinit(user_mgr.stt_session);
    llm_tts_deinit(user_mgr.tts_session);
    llm_global_deinit();
}

struct os_task llm_task_demo;
void llm_demo(void)
{
    OS_TASK_INIT("LLM_DEMO", &llm_task_demo, llm_main_demo, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
}

#endif

