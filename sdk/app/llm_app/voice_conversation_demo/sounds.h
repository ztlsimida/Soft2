#ifndef _SOUNDS_H_
#define _SOUNDS_H_

#include "sys_config.h"

// 声明数组
extern const char yilianjie[];
extern unsigned int yilianjie_size;

#ifdef LLM_SPV12XX
extern const char wozai[];
extern unsigned int wozai_size;
#endif

// 如果还有其他数组，也可以在这里声明
// extern const unsigned char another_sound[];
// extern const unsigned int another_sound_size;

#endif // SOUNDS_H

