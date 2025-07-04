#ifndef _LLM_API_H_
#define _LLM_API_H_

#include "basic_include.h"
#include "lib/net/llm/llm_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LLM_SESSION_MAX (3)

struct llm_data;
struct llm_model_data;

typedef int32(*llm_evt_cb)(void *session, uint16 evt, uint32 param1, uint32 param2);

enum llm_event {
    //general
    LLM_EVENT_STT_RESULT,
    LLM_EVENT_TTS_RESULT,
    LLM_EVENT_CHAT_RESULT,
    LLM_EVENT_ERROR_MSG,
    LLM_EVENT_TIMEOUT,

    //llm_tts
    LLM_EVENT_TTS_AUDIO_START,
    LLM_EVENT_TTS_AUDIO_ERR,

    //llm_chat
    LLM_EVENT_CHAT_CONTEXT_ID,
    
    //llm_sts
    LLM_EVENT_CONNECTED,
    LLM_EVENT_DISCONNECT,
    LLM_EVENT_VAD_RESULT,
    LLM_EVENT_FINISH_RESULT,
};

typedef enum {
    LLM_CONFIG_TYPE_MODEL,
    LLM_CONFIG_TYPE_TRANS,
} llm_cfg_type;

typedef enum {
    LLM_STATR_TRANSFER = 1,
    LLM_TRANSFERRING,
    LLM_STOP_TRANSFER,
} llm_trans_state;

struct llm_model {
    const char *name;
    const struct llm_model_data *m;
};

struct llm_global_param {
    uint16 task_reuse: 1, rev: 15;
    uint16 model_count;
    const struct llm_model *models;
};

struct llm_trans_param {
    long        low_speed_limit;
    long        low_speed_time;
    long        connect_timeout;
    size_t      blob_len;
    void        *blob_data;
};

struct llm_chat_sparam {
    uint16 qmsg_tx_cnt;
    uint16 qmsg_rx_cnt;
    uint16 reply_fragment_size;
    uint16 context_id_len;
    void   *evt_cb;
};

struct llm_stt_sparam {
    uint16 qmsg_tx_cnt;
    uint16 qmsg_rx_cnt;
    uint16 cb_max_size;
    void   *evt_cb;
};

struct llm_tts_sparam {
    uint16 qmsg_tx_cnt;
    uint16 rx_buff_size;
    uint16 cb_max_size;
    void   *evt_cb;
};

struct llm_sts_sparam {
    uint16 audio_url_cnt;
    uint16 qmsg_tx_cnt;
    uint16 rx_buff_size;
    uint16 cb_max_size;
    uint16 retry_cnt;
    void   *evt_cb;
};

int32 llm_global_init(struct llm_global_param *global);
int32 llm_global_deinit(void);

//CHAT
void *llm_chat_init(char *llm_name, struct llm_chat_sparam *sparam);
int32 llm_chat_deinit(void *chat_hdl);
int32 llm_chat_send(void *chat_hdl, char *question, uint32 question_len);
int32 llm_chat_recv(void *chat_hdl, char *buff, uint32 buff_size);
int32 llm_chat_set_role(void *chat_hdl, char *role);
int32 llm_chat_set_prefill(void *chat_hdl, char *prefill);
int32 llm_chat_change_dialogue(void *chat_hdl, char *context_id, uint32 content_len);
int32 llm_chat_new_dialogue(void *chat_hdl, char *cache_mode);
int32 llm_chat_config(void *chat_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_chat_stop(void *chat_hdl);

//STT
void *llm_stt_init(char *llm_name, struct llm_stt_sparam *sparam);
int32 llm_stt_deinit(void *stt_hdl);
int32 llm_stt_send(void *stt_hdl, char *audio, uint32 audio_len, llm_trans_state state);
int32 llm_stt_recv(void *stt_hdl, char *buff, uint32 buff_size);
int32 llm_stt_config(void *stt_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_stt_stop(void *stt_hdl);

//TTS
void *llm_tts_init(char *llm_name, struct llm_tts_sparam *sparam);
int32 llm_tts_deinit(void *tts_hdl);
int32 llm_tts_send(void *tts_hdl, char *text, uint32 text_len, llm_trans_state state);
int32 llm_tts_recv(void *tts_hdl, char *buff, uint32 buff_size);
int32 llm_tts_config(void *tts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_tts_stop(void *tts_hdl);
int32 llm_tts_play_audio(void *tts_hdl, const       char *audio, uint32 length);

//STS
void *llm_sts_init(char *llm_name, struct llm_sts_sparam *sparam);
int32 llm_sts_deinit(void *sts_hdl);
int32 llm_sts_play_audio(void *sts_hdl, const       char *audio, uint32 length);
int32 llm_sts_check_audio(void *sts_hdl);
int32 llm_sts_send(void *sts_hdl, char *audio, uint32 audio_len, llm_trans_state state);
int32 llm_sts_recv(void *sts_hdl, char *buff, uint32 buff_size);
int32 llm_sts_wait(void *sts_hdl, int32 timeout);
int32 llm_sts_config(void *sts_hdl, llm_cfg_type type, void *cfg, uint32 cfg_size);
int32 llm_sts_stop(void *sts_hdl);

/////////////////////////////////////////////////////////////////////
//doubao
#include "lib/net/llm/doubao/doubao_chat.h"
#include "lib/net/llm/doubao/doubao_stt.h"
#include "lib/net/llm/doubao/doubao_tts.h"

//listenai
#include "lib/net/llm/listenai/listenai.h"

//coze
#include "lib/net/llm/coze/coze.h"

/////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

