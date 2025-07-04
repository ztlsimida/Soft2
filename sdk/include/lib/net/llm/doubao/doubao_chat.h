#ifndef _DOUBAO_CHAT_H_
#define _DOUBAO_CHAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONTEXT_MODE_SESSION "session"
#define CONTEXT_MODE_PREFIX  "common_prefix"

struct Doubao_CHAT_Cfg {
    char    *chat_url;               //必填
    char    *ark_api_key;            //必填
    char    *endpoint_id;            //必填

    char    *stop;                   //可选
    char    *max_tokens;             //可选

    char    *context_url;            //若使能上下文缓存，则需设置
    char    *context_chat_url;       //若使能上下文缓存，则需设置
    char    *ttl;                    //若使能上下文缓存，则需设置
    char    *last_history_tokens;    //若上下文缓存模式选择 session，则需设置
};
extern const struct llm_model_data doubao_llm_chat;

#ifdef __cplusplus
}
#endif

#endif


