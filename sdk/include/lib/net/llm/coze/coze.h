#ifndef _COZE_STS_H_
#define _COZE_STS_H_

#ifdef __cplusplus
extern "C" {
#endif

//详细配置说明文档请查阅https://www.coze.cn/open/docs/developer_guides/streaming_chat_event#91642fa8
struct coze_chat_platform_cfg {
    char    *host_url;              //必填
    char    *pat_key;               //必填
    char    *bot_id;                //必填
//  音频输入参数
    char    *input_audio_codec;     //可选,输入音频的编码，支持 pcm、opus、g711a、g711u。默认为 pcm。                                    //如果音频编码格式为 g711a 或 g711u，format 请设置为 pcm。
    char    *input_audio_sample_rate;    //可选,输入音频的采样率，默认是 24000。支持 8000、16000、22050、24000、32000、44100、48000。如果音频编码格式 codec 为 g711a 或 g711u，音频采样率需设置为 8000。                            
    char    *input_audio_channel;   //可选，输入音频的声道数，支持 1（单声道）、2（双声道）。默认是 1（单声道）。
    char    *input_audio_bit_depth; //可选，输入音频的位深，默认是 16，支持8、16和24。 
//  音频输出参数    
    char    *output_audio_codec;    //可选,输出音频编码，支持 pcm、opus。默认是 pcm。
    char    *output_audio_pcm_config_sample_rate;//可选，输出 pcm 音频的采样率，默认是 8000。支持  8000、16000、22050、24000、32000、44100、48000。仅输出为PCM时间有效
    char    *output_audio_pcm_config_frame_size_ms;//可选，输出每个 pcm 包的时长，单位 ms，默认50。

    char    *output_audio_opus_config_bitrate;  //可选，输出 opus 的码率，默认 8000。
    char    *output_audio_opus_config_frame_size_ms; //可选，输出 opus 的帧长，默认是 60。可选值：2.5、5、10、20、40、60

    char    *max_frame_num;  //可选，2s周期最大返回的帧数量。
    
    char    *output_audio_speech_rate; //可选，输出音频的语速，取值范围 [-50, 100]，默认为 0。-50 表示 0.5 倍速，100 表示 2 倍速。
    char    *voice_id;              //可选，音色
    char    *prologue_content;      //可选,自定义开场白，如果不设定自定义开场白则使用智能体上设置的开场白。
//打断参数
    char    *turn_detection_prefix_padding_ms;//可选，SERVER VAD 检测到语音之前要包含的音频量，单位为 ms。默认为 600ms。
    char    *turn_detection_silence_duration_ms;//可选，检测语音停止的静音持续时间，单位为 ms。默认为 500ms。
    char    *turn_detection_interrupt_config_mode;    //可选，keyword_contains模式下，说话内容包含关键词才会打断模型回复。例如关键词"扣子"，用户正在说“你好呀扣子......” / “扣子你好呀”，模型回复都会被打断。keyword_prefix模式下，说话内容前缀匹配关键词才会打断模型回复。例如关键词"扣子"，用户正在说“扣子你好呀......”，模型回复就会被打断，而用户说“你好呀扣子......”，模型回复不会被打断。

//其他asr参数
    char    *asr_config_user_language;//可选，用户说话的语种，默认为 common。选项包括： common：大模型语音识别，可自动识别中英粤。
                                        //zh：小模型语音识别，中文。
                                        //cant：小模型语音识别，粤语。
                                        //sc：小模型语音识别，川渝。
                                        //en：小模型语音识别，英语。
                                        //ja：小模型语音识别，日语。
                                        //ko：小模型语音识别，韩语。
                                        //fr：小模型语音识别，法语。
                                        //id：小模型语音识别，印尼语。
                                        //es：小模型语音识别，西班牙语。
                                        //pt：小模型语音识别，葡萄牙语。
                                        //ms：小模型语音识别，马来语。
                                        //ru：小模型语音识别，俄语。
};

extern const struct llm_model_data coze_sts_model; 

enum coze_server_vad_event{
    COZE_SERVER_VAD_START = 1,
    COZE_SERVER_VAD_STOP,
};

#ifdef __cplusplus
}
#endif

#endif




