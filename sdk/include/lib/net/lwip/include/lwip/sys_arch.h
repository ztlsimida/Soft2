/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_SYS_ARCH_H
#define LWIP_HDR_SYS_ARCH_H

/* unify os includes. */
#include "osal/msgqueue.h"
#include "osal/mutex.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/timer.h"
#include "lwip/opt.h"
#include "lwip/arch.h"

#include "hal/dma.h"

typedef u32_t sys_prot_t;

//typedef int sys_sem_t;
typedef struct {
    struct os_semaphore sem;                ///< for unify lwip,use os_semaphore instead
    u8_t sem_vaild;                  ///< FALSE means this sem invaild,TRUE means this sem vaild.
} sys_sem_t;

#define sys_sem_valid(sema) (((sema) != NULL) && ((((*sema).sem_vaild) == FALSE) ? 0 : 1))

//typedef int sys_mutex_t;
typedef struct {
    struct os_mutex mutex;
    u8_t mutex_vaild;
} sys_mutex_t;

#define sys_mutex_valid(mutex) (((mutex) != NULL) && ((((*mutex).mutex_vaild) == FALSE) ? 0 : 1))
/*
struct lwip_mbox {
  void* sem;
  void** q_mem;
  unsigned int head, tail;
  int size;
  int used;
};
*/
typedef struct {
    struct os_msgqueue msgq;              ///< for unify lwip,use os_semaphore instead
    u8_t mbox_vaild;
} sys_mbox_t;


#define SYS_MBOX_NULL NULL
#define sys_mbox_valid(mbox) (((mbox) != NULL) && ((((*mbox).mbox_vaild) == FALSE) ? 0 : 1 ))
#define sys_mbox_valid_val(mbox) sys_mbox_valid(&(mbox))

typedef struct os_task *sys_thread_t;

//#define SYS_ARCH_DECL_PROTECT(lev)  
//#define SYS_ARCH_PROTECT(lev)
//#define SYS_ARCH_UNPROTECT(lev)

/* to implement doing something while blocking on an mbox or semaphore:
 * pass a function to test_sys_arch_wait_callback() that returns
 * '0' if waiting again and
 * '1' if now there should be something to do (used for asserting)
 */
typedef int (*test_sys_arch_waiting_fn)(sys_sem_t* wait_sem, sys_mbox_t* wait_mbox);
void test_sys_arch_wait_callback(test_sys_arch_waiting_fn waiting_fn);

/* current time */
extern u32_t lwip_sys_now;


#endif /* LWIP_HDR_TEST_SYS_ARCH_H */

