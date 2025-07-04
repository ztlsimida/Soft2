#ifndef __VOICE_CONVERSATION_DEMO_H
#define __VOICE_CONVERSATION_DEMO_H

#include "llm/llm_api.h"

#include "syscfg.h"
#include "keyWork.h"
#include "keyScan.h"
#include "mp3_decode.h"
#include "audio_dac.h"
#include "stream_define.h"
#include "stream_frame.h"
#include "lwip/netdb.h"

#define STT_RECV_SIZE   1024
#define CHAT_RECV_SIZE  1024

struct llm_demo_manage {
    uint8   key_transfer:1, key_transfer_old: 1, auadc_model_start:1, error: 1, rev: 4;
    struct  os_mutex lock;

    void    *chat_session;
    void    *stt_session;
    void    *tts_session;

    char    *stt_recv;
    char    *chat_recv;
    int32   stt_rx_len;
    int32   chat_rx_len;

    RBUFFER_DEF_R(chat_resp, char *);
};

#endif
