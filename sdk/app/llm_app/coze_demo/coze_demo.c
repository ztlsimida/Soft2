#include "coze_demo.h"
#include "sounds.h"

#ifdef COZE_DEMO

/* Please add your server information */
#define COZE_URL        "wss://ws.coze.cn/v1/chat?bot_id="
#define COZE_BOT_ID     "xxx"
#define COZE_PAT_KEY    "xxx"

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif
#define COZE_IDLE_TIMEOUT 20000
#define COZE_DNS_TIMEOUT    10000
#define OPUS_PROMPT_FRAME_LEN 60

static int32 coze_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2);

const struct llm_model models[] = {
    {"coze_sts", (struct llm_model_data *) &coze_sts_model},
};

struct llm_global_param global = {
    .task_reuse     = 0,
    .model_count    = 1,
    .models         = models,
};

struct llm_sts_sparam coze_sts_session_cfg = {
    .audio_url_cnt          = 1,//not need yet
    .qmsg_tx_cnt            = 16,//audio sample msg num
    .rx_buff_size           = 4096,//audeo recv buff
    .cb_max_size            = 2048,
    .retry_cnt              = 2,
    .evt_cb                 = coze_event_cb,
};

struct coze_chat_platform_cfg coze_sts_platform_cfg = {
    .host_url               = COZE_URL,
    .pat_key                = COZE_PAT_KEY,
    .bot_id                 = COZE_BOT_ID,
    .input_audio_codec      = "g711a",
    .output_audio_codec     = "opus",
};
struct llm_trans_param coze_sts_trans_cfg = {
    .low_speed_limit    = 10,
    .low_speed_time     = 5,
    .connect_timeout    = 5,
    .blob_len           = 0,
    .blob_data          = NULL,
};

struct coze_demo_manage coze_mgr = {
    .key_transfer       = 0,
    .auadc_model_start  = 0,
    .connected          = 0,
    .finish             = 0,
    .timeout            = 0,
};

struct os_task coze_task_demo;
struct os_task coze_audio_play_task;

static void coze_sts_play_opus(const char *audio, uint16 audio_len, uint32 frame_len)
{
    uint16 xfer_len = frame_len;
    uint16 copy_len = audio_len;
    char *write_addr = (char *)audio;

    while (copy_len > 0) {
        llm_sts_play_audio(coze_mgr.sts_session, &xfer_len, 2);
        llm_sts_play_audio(coze_mgr.sts_session, write_addr, xfer_len);
        write_addr   += xfer_len;
        copy_len     -= xfer_len;
    }
}

int32 coze_event_cb(void *session, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case LLM_EVENT_CONNECTED:
            os_printf("Coze AI connected!\r\n");
            coze_mgr.connected = 1;
            coze_mgr.connect_audio_play = 1;
            coze_mgr.time_stamp = os_jiffies();
            break;
        case LLM_EVENT_DISCONNECT:
            os_printf("Coze AI disconnect!\r\n");
            coze_mgr.connected = 0;
            coze_mgr.disconnect_audio_play = 1;
            break;
        case LLM_EVENT_STT_RESULT:
            os_printf("STT(%d):", param2);
            hgprintf_out((char *)param1, param2, 0);
            _os_printf("\r\n");
            break;
        case LLM_EVENT_TTS_RESULT:
            //os_printf("TTS(%d):", param2));
            //hgprintf_out((char *)param1, param2, 0);
            //_os_printf("\r\n");
            break;
        case LLM_EVENT_VAD_RESULT:
            os_printf("Coze AI recv server vad result:%s\r\n", param1 == 1 ? "START" : "STOP");
            coze_mgr.time_stamp = UINT64_MAX;
            if (param1 == COZE_SERVER_VAD_STOP) {
                coze_mgr.auadc_model_start = 0;
            }
            break;
        case LLM_EVENT_FINISH_RESULT:
            os_printf("COZE AI check finish!\r\n");
            coze_mgr.finish = 1;
            coze_mgr.time_stamp = os_jiffies();
            break;
        case LLM_EVENT_ERROR_MSG:
            os_printf("ERR_MSG:%s\r\n", (char *)param1);
            break;
        default:
            break;
    }
    return RET_OK;
}

#ifdef LLM_SPV12XX
struct os_task coze_task_awaken;

#define DEBOUNCE_COUNT  3       // 去抖动计数器阈值，连续检测到相同状态3次认为有效
#define COOLDOWN_TIME   3000    // 冷却时间，单位为毫秒（3秒）

void coze_awaken_demo(void)
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

                        coze_mgr.key_transfer = 1;
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
uint32_t coze_intercom_push_key(struct key_callback_list_s *callback_list, uint32_t keyvalue, uint32_t extern_value)
{
    if ((keyvalue >> 8) != AD_PRESS)
    { return 0; }
    uint32 key_val = (keyvalue & 0xff);
    if (key_val == KEY_EVENT_DOWN) {
        os_printf(KERN_ERR"Wakeup triggered!\r\n");
        coze_mgr.key_transfer = 1;
    }
    return 0;
}
#endif

static int32 coze_opcode_func(stream *s, void *priv, int opcode)
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

int32 coze_app_init(char *llm_name)
{
    int ret = 0;
    coze_mgr.sts_session = llm_sts_init(llm_name, &coze_sts_session_cfg);
    if (!coze_mgr.sts_session) {
        os_printf("sts session init fail!\r\n");
        return RET_ERR;
    }

    ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_TRANS,
                         (void *)&coze_sts_trans_cfg, sizeof(coze_sts_trans_cfg));
    if (ret) {
        os_printf("sts_trans_cfg fail!\r\n");
        return RET_ERR;
    }

    ret = llm_sts_config(coze_mgr.sts_session, LLM_CONFIG_TYPE_MODEL,
                         (void *)&coze_sts_platform_cfg, sizeof(coze_sts_platform_cfg));
    if (ret) {
        os_printf("asr_sts_cfg fail!\r\n");
        return RET_ERR;
    }
    return ret;
}

static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure *)data;
    return (uint32_t)d->priv;
}

static uint32_t set_sound_data_len(void *data, uint32_t len)
{
    struct data_structure  *d = (struct data_structure *)data;
    d->priv = (void *)len;
    return (uint32_t)len;
}

static const stream_ops_func stream_sound_ops = {
    .get_data_len = get_sound_data_len,
    .set_data_len = set_sound_data_len,
};

static int coze_audio_play_opcode_func(stream *s, void *priv, int opcode)
{
    static uint8_t *audio_buf = NULL;
    int res = 0;
    switch (opcode) {
        case STREAM_OPEN_EXIT: {
            audio_buf = os_malloc(3 * 60 * 8 * 2);  //帧长 * 每毫秒采样率 * 两个byte
            if (audio_buf) {
                stream_data_dis_mem(s, 3);
            }
            streamSrc_bind_streamDest(s, R_SPEAKER);
        }
        break;
        case STREAM_DATA_DIS: {
            struct data_structure *data = (struct data_structure *)priv;
            int data_num = (int)data->priv;
            data->ops = (stream_ops_func *)&stream_sound_ops;
            data->data = audio_buf + (data_num) * 60 * 16;
        }
        break;
        case STREAM_DATA_DESTORY: {
            if (audio_buf) {
                os_free(audio_buf);
                audio_buf = NULL;
            }
        }
        case STREAM_DATA_FREE:
            //_os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            break;


        //数据发送完成,可以选择唤醒对应的任务
        case STREAM_RECV_DATA_FINISH:
            break;

        default:
            //默认都返回成功
            break;
    }
    return res;
}

void coze_audio_play_thread(void *arg)
{
    struct data_structure *data_s = NULL;
    stream *s = NULL;
    OpusDecoder *decoder = NULL;
    uint8_t *opus_buffer = NULL;
    uint16_t encode_len = 0;
    uint32_t dec_len = 0;
    int read_len = 0;

    s = open_stream_available("file_audio", 3, 0, coze_audio_play_opcode_func, NULL);
    audio_dac_set_filter_type(SOUND_FILE);

    int err = 0;;
    if (!(decoder = opus_decoder_create(8000, 1, &err))) {
        os_printf("Opus decoder init failed: %d\n", err);
        goto cleanup;
    }

    while (1) {
        os_sleep_ms(1);

        /* 数据获取阶段 */
        if (!(data_s = get_src_data_f(s))) { continue; }
        char *pcm_data = get_stream_real_data(data_s);

        /* 协议头解析阶段 */
        if ((read_len = llm_sts_recv(coze_mgr.sts_session, (char *)&encode_len, sizeof(encode_len))) != sizeof(encode_len)) {
            goto data_cleanup;
        }

        /* 数据有效性检查 */
        if (encode_len <= 0) {
            goto data_cleanup;
        }

        /* 缓冲区动态管理 */
        if (!(opus_buffer = os_realloc(opus_buffer, encode_len))) {
            os_printf("Buffer allocation failed for %d bytes\n", encode_len);
            goto data_cleanup;
        }

        /* 数据接收阶段 */
        if ((read_len = llm_sts_recv(coze_mgr.sts_session, (char *)opus_buffer, encode_len)) != encode_len) {
            os_printf("Data incomplete: %d/%d\n", read_len, encode_len);
            goto data_cleanup;
        }

        /* 音频解码阶段 */
        if ((dec_len = opus_decode(decoder, opus_buffer, encode_len,
                                   pcm_data, 60 * 8, 0)) <= 0) {
            os_printf("Decode error: %d\n", dec_len);
            goto data_cleanup;
        }

        /* 数据发送阶段 */
        data_s->type = SET_DATA_TYPE(SOUND, SOUND_FILE);
        set_sound_data_len(data_s, 60 * 8 * 2);
        send_data_to_stream(data_s);
        data_s = NULL;
        continue;

data_cleanup:
        force_del_data(data_s);
        data_s = NULL;
    }

cleanup:
    if (decoder) {
        opus_decoder_destroy(decoder);
    }
    if (opus_buffer) {
        os_free(opus_buffer);
    }
}

void llm_free_sample_stream(stream *s)
{
    struct data_structure *data_s = NULL;
    do {
        data_s = recv_real_data(s);
        if (data_s) {
            free_data(data_s);
        }
    } while (data_s != NULL);
}

void coze_main_demo(void)
{
    int32 result = RET_OK;
    struct data_structure *data_s = NULL;
    char *data = NULL;
    uint32 data_len = 0;
    stream *s = NULL;
    char *aualaw_buf = NULL;
    uint64 jiff = 0;
#ifdef PSRAM_HEAP
    char *psram_copy_buff = NULL;
#endif

    os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");

    gethostbyname_async("ws.coze.cn");
    coze_mgr.dns_time = os_jiffies();

    llm_global_init(&global);

    result = coze_app_init("coze_sts");
    if (result) {
        os_printf("STS session init failed:%d\n", result);
        goto cleanup;
    }

    s = open_stream_available(R_SPEECH_RECOGNITION, 0, 8, coze_opcode_func, NULL);
    if (!s) {
        os_printf("open speech_recognition stream err!\r\n");
        goto cleanup;
    }

#ifndef LLM_SPV12XX
    add_keycallback(coze_intercom_push_key, NULL);
#else
    OS_TASK_INIT("COZE_AWAKEN", &coze_task_awaken, coze_awaken_demo, NULL, OS_TASK_PRIORITY_NORMAL + 1, 1024);
#endif

    struct aualaw_device *aualaw_dev = (struct aualaw_device *)dev_get(HG_AUALAW_DEVID);
    result = aualaw_open(aualaw_dev);
    if (result != 0) {
        os_printf("aualaw open err!\n");
        goto cleanup;
    }

    audac_enable_play();

    OS_TASK_INIT("COZE_AUPLAY", &coze_audio_play_task, coze_audio_play_thread, NULL, OS_TASK_PRIORITY_NORMAL, 8192);

    while (1) {
startup:
        if (coze_mgr.connected == 1) {
            if (coze_mgr.connect_audio_play == 1) {
                audac_disable_play();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                coze_sts_play_opus(yilianjie, yilianjie_size, OPUS_PROMPT_FRAME_LEN);
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                coze_mgr.connect_audio_play = 0;
            }

            if (coze_mgr.key_transfer) {
                coze_mgr.auadc_model_start = 0;
                llm_sts_stop(coze_mgr.sts_session);
                audac_disable_play();
                //mp3_decode_clean_stream();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                do {
                    result = llm_sts_wait(coze_mgr.sts_session, 500);
                    if (coze_mgr.connected == 0) {
                        os_printf("This can be confirmed to be a disconnection!\r\n");
                        goto startup;
                    }
                    if (result != RET_OK && result != LLME_WAIT_TIMEOUT) {
                        os_printf("Maybe it's disconnected?\r\n");
                        goto startup;
                    }
                } while (result == LLME_WAIT_TIMEOUT);
                //llm_sts_play_audio(coze_mgr.sts_session, wozai, wozai_size);
                coze_sts_play_opus(wozai, wozai_size, OPUS_PROMPT_FRAME_LEN);
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                os_printf("%s,%d:Please speak...auadc_model_start:%d\r\n", __FUNCTION__, __LINE__, coze_mgr.auadc_model_start);
                os_sleep_ms(1000);
                llm_free_sample_stream(s);
                coze_mgr.auadc_model_start = 1;
                coze_mgr.finish = 0;
                coze_mgr.key_transfer = 0;
                coze_mgr.time_stamp = os_jiffies();
            }

            jiff = os_jiffies() - coze_mgr.time_stamp;
            if (os_jiffies_to_msecs(jiff) > COZE_IDLE_TIMEOUT && coze_mgr.time_stamp != UINT64_MAX) {
                llm_sts_stop(coze_mgr.sts_session);
                audac_disable_play();
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                audac_enable_play();
                coze_sts_play_opus(tuixia, tuixia_size, OPUS_PROMPT_FRAME_LEN);
                while (!audac_wait_empty()) {
                    os_sleep_ms(1);
                }
                coze_mgr.auadc_model_start = 0;
                coze_mgr.finish = 0;
                coze_mgr.time_stamp = UINT64_MAX;
            }

            if (coze_mgr.finish) {
                if (llm_sts_check_audio(coze_mgr.sts_session)) {
                    do {
                        result = llm_sts_wait(coze_mgr.sts_session, 500);
                        if (coze_mgr.connected == 0) {
                            os_printf("This can be confirmed to be a disconnection!\r\n");
                            goto startup;
                        }
                        if (result != RET_OK && result != LLME_WAIT_TIMEOUT) {
                            os_printf("Maybe it's disconnected?\r\n");
                            goto startup;
                        }
                    } while (result == LLME_WAIT_TIMEOUT);
                    os_printf("new dialogue.\r\n");
                    os_printf("%s,%d:Please speak...\r\n", __FUNCTION__, __LINE__);                           
                    os_sleep_ms(1000);
                    llm_free_sample_stream(s);
                    coze_mgr.auadc_model_start = 1;
                    coze_mgr.finish = 0;
                    coze_mgr.time_stamp = os_jiffies();
                }
            }

            data_s = recv_real_data(s);
            if (data_s) {
                data = get_stream_real_data(data_s);
                data_len = get_stream_real_data_len(data_s);
                if (!aualaw_buf)
                { aualaw_buf = (char *)os_malloc(data_len / 2); }
#ifdef PSRAM_HEAP
                if (!psram_copy_buff) {
                    psram_copy_buff = os_malloc(data_len);
                }
                if (psram_copy_buff) {
                    memcpy(psram_copy_buff, data, data_len);
                    result = aualaw_encode(aualaw_dev, psram_copy_buff, data_len, aualaw_buf);
                }
#else
                result = aualaw_encode(aualaw_dev, data, data_len, aualaw_buf);
#endif
                if (result != 0) {
                    os_printf("aualaw encode err!\n");
                    free_data(data_s);
                    break;
                }
                if (coze_mgr.auadc_model_start) {
                    if (data) {
                        llm_sts_send(coze_mgr.sts_session, aualaw_buf, data_len / 2, LLM_TRANSFERRING);
                        //os_printf("send audio len:%d\r\n", data_len / 2);
                    }
                }
                free_data(data_s);
                data_s = NULL;
            }
        } else {
            if (coze_mgr.disconnect_audio_play == 1) {
                audac_disable_play();
                audac_enable_play();
                //llm_sts_play_audio(coze_mgr.sts_session, yiduankai, yiduankai_size);
                //coze_sts_play_opus(yiduankai, yiduankai_size, OPUS_PROMPT_FRAME_LEN);
                os_printf("****ALREADY DISCONNECTED!\n");
                coze_mgr.disconnect_audio_play = 0;
            }

            if (coze_mgr.key_transfer == 1) {
                os_printf("***Network too bad,wait for reconnect！\r\n");
                coze_mgr.key_transfer = 0;
            }
            if (coze_mgr.auadc_model_start == 1) {
                coze_mgr.auadc_model_start = 0;
            }
            if (coze_mgr.finish == 1) {
                coze_mgr.finish = 0;
            }
        }
        os_sleep_ms(10);
    }

cleanup:
    llm_sts_deinit(coze_mgr.sts_session);
    llm_global_deinit();
}


void coze_demo(void)
{
    OS_TASK_INIT("COZE_DEMO", &coze_task_demo, coze_main_demo, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
}

#endif

