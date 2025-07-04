#ifndef __LISTENAI_DEMO_H
#define __LISTENAI_DEMO_H
#include "llm/llm_api.h"

#include "syscfg.h"
#include "keyWork.h"
#include "keyScan.h"
#include "mp3_decode.h"
#include "audio_dac.h"
#include "stream_define.h"
#include "stream_frame.h"
#include "lwip/netdb.h"

struct listenai_demo_manage {
    uint8   key_transfer:1, auadc_model_start:1, finish: 1, connected: 1, timeout: 2, 
            connect_audio_play: 1, disconnect_audio_play: 1;
    void    *sts_session;
};

#endif


