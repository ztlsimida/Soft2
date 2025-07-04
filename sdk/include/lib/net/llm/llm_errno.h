#ifndef _LLM_ERRNO_H_
#define _LLM_ERRNO_H_

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef RET_OK
#define RET_OK  0
#endif

#ifndef RET_ERR
#define RET_ERR -1
#endif

#ifndef LLME_AGAIN
#define LLME_AGAIN  1
#endif

#ifndef LLME_NOMEM
#define LLME_NOMEM  2
#endif

#ifndef LLME_INTR
#define LLME_INTR   3
#endif

#ifndef LLME_CONN_FAIL
#define LLME_CONN_FAIL  4
#endif
    
#ifndef LLME_SEND_FAIL
#define LLME_SEND_FAIL  5
#endif
    
#ifndef LLME_RECV_FAIL
#define LLME_RECV_FAIL  6
#endif

#ifndef LLME_CONN_TIMEOUT
#define LLME_CONN_TIMEOUT  7
#endif

#ifndef LLME_SEND_TIMEOUT
#define LLME_SEND_TIMEOUT  8
#endif

#ifndef LLME_RECV_TIMEOUT
#define LLME_RECV_TIMEOUT  9
#endif

#ifndef LLME_WAIT_TIMEOUT
#define LLME_WAIT_TIMEOUT  10
#endif

#ifdef __cplusplus
}
#endif

#endif

