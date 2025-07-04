#include "sys_config.h"
#include "typesdef.h"
#include "osal/string.h"
#include "lib/heap/sysheap.h"
#include "lib/common/rbuffer.h"

#if MPOOL_ALLOC

__bobj struct sys_sramheap sram_heap;

void *_os_malloc(int size)
{
    void *ptr = sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc sram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void _os_free(void *ptr)
{
    if(ptr){
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (sysheap_free(&sram_heap, ptr)) {
            os_printf(KERN_WARNING"free sram error, ptr=%p, [LR:%p]\r\n", ptr, RETURN_ADDR());
        }
    }
}

void *_os_zalloc(int size)
{
    void *ptr = sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
        os_memset(ptr, 0, size);
    } else {
        // os_printf(KERN_WARNING"malloc sram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void *_os_calloc(size_t nmemb, size_t size)
{
    void *ptr = sysheap_alloc(&sram_heap, nmemb * size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, nmemb * size);
        }
        os_memset(ptr, 0, nmemb * size);
    } else {
        // os_printf(KERN_WARNING"malloc sram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void *_os_realloc(void *ptr, int size)
{
    void *nptr = sysheap_alloc(&sram_heap, size, RETURN_ADDR(), 0);
    if (nptr) {
        ASSERT((uint32)nptr == ALIGN((uint32)nptr, sram_heap.pool.align));
        if (IS_DCACHE_ADDR(nptr)) {
            sys_dcache_clean_invalid_range(nptr, size);
        }
        if(ptr){
            os_memcpy(nptr, ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc sram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }

    if(ptr){
        _os_free(ptr);
    }
    return nptr;
}

void *_os_malloc_t(int size, const char *func, int line)
{
    void *ptr = sysheap_alloc(&sram_heap, size, func, line);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc sram fail, size=%d [%s:%d]\r\n", size, func, line);
    }
    return ptr;
}

void _os_free_t(void *ptr, const char *func, int line)
{
    if(ptr){
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, sram_heap.pool.align));
        if (sysheap_free(&sram_heap, ptr)) {
            os_printf(KERN_WARNING"free sram error, ptr=%p, [%s:%d]\r\n", ptr, func, line);
        }
    }
}

void *_os_zalloc_t(int size, const char *func, int line)
{
    void *ptr = _os_malloc_t(size, func, line);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc_t(size_t nmemb, size_t size, const char *func, int line)
{
    return _os_zalloc_t(nmemb * size, func, line);
}

void *_os_realloc_t(void *ptr, int size, const char *func, int line)
{
    void *nptr = _os_malloc_t(size, func, line);
    if (nptr && ptr) {
        os_memcpy(nptr, ptr, size);
    }
    if(ptr){
        _os_free_t(ptr, func, line);
    }
    return nptr;
}

#ifdef MALLOC_IN_PSRAM
void *malloc(uint32 size)               __alias(_os_malloc_psram);
void  free(void *ptr)                   __alias(_os_free_psram);
void *zalloc(size_t size)               __alias(_os_zalloc_psram);
void *calloc(size_t nmemb, size_t size) __alias(_os_calloc_psram);
void *realloc(void *ptr, size_t size)   __alias(_os_realloc_psram);
#else
void *malloc(uint32 size)               __alias(_os_malloc);
void  free(void *ptr)                   __alias(_os_free);
void *zalloc(size_t size)               __alias(_os_zalloc);
void *calloc(size_t nmemb, size_t size) __alias(_os_calloc);
void *realloc(void *ptr, size_t size)   __alias(_os_realloc);
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef PSRAM_HEAP // PSRAM heap 相关API
__bobj struct sys_psramheap psram_heap;

void *_os_malloc_psram(int size)
{
    void *ptr = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc psram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void _os_free_psram(void *ptr)
{
    if(ptr){
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (sysheap_free(&psram_heap, ptr)) {
            os_printf(KERN_WARNING"free psram error, ptr=%p [LR:%p]\r\n", ptr, RETURN_ADDR());
        }
    }
}

void *_os_zalloc_psram(size_t size)
{
    void *ptr = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
        os_memset(ptr, 0, size);
    } else {
        // os_printf(KERN_WARNING"malloc psram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void *_os_calloc_psram(size_t nmemb, size_t size)
{
    void *ptr = sysheap_alloc(&psram_heap, nmemb * size, RETURN_ADDR(), 0);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, nmemb * size);
        }
        os_memset(ptr, 0, nmemb * size);
    } else {
        // os_printf(KERN_WARNING"malloc psram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }
    return ptr;
}

void *_os_realloc_psram(void *ptr, int size)
{
    void *nptr = sysheap_alloc(&psram_heap, size, RETURN_ADDR(), 0);
    if (nptr) {
        ASSERT((uint32)nptr == ALIGN((uint32)nptr, psram_heap.pool.align));
        if (IS_DCACHE_ADDR(nptr)) {
            sys_dcache_clean_invalid_range(nptr, size);
        }
        if(ptr){
            os_memcpy(nptr, ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc psram fail, size=%d [LR:%p]\r\n", size, RETURN_ADDR());
    }

    if(ptr){
        _os_free_psram(ptr);
    }
    return nptr;
}

void *_os_malloc_psram_t(int size, const char *func, int line)
{
    void *ptr = sysheap_alloc(&psram_heap, size, func, line);
    if (ptr) {
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (IS_DCACHE_ADDR(ptr)) {
            sys_dcache_clean_invalid_range(ptr, size);
        }
    } else {
        // os_printf(KERN_WARNING"malloc psram fail, size=%d [%s:%d]\r\n", size, func, line);
    }
    return ptr;
}

void _os_free_psram_t(void *ptr, const char *func, int line)
{
    if(ptr){
        ASSERT((uint32)ptr == ALIGN((uint32)ptr, psram_heap.pool.align));
        if (sysheap_free(&psram_heap, ptr)) {
            os_printf(KERN_WARNING"_os_free_psram error, ptr=%p, [%s:%d]\r\n", ptr, func, line);
        }
    }
}

void *_os_zalloc_psram_t(int size, const char *func, int line)
{
    void *ptr = _os_malloc_psram_t(size, func, line);
    if (ptr) {
        os_memset(ptr, 0, size);
    }
    return ptr;
}

void *_os_calloc_psram_t(size_t nmemb, size_t size, const char *func, int line)
{
    return _os_zalloc_psram_t(nmemb * size, func, line);
}

void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line)
{
    void *nptr = _os_malloc_psram_t(size, func, line);
    if (nptr && ptr) {
        os_memcpy(nptr, ptr, size);
    }
    if(ptr){
        _os_free_psram_t(ptr, func, line);
    }
    return nptr;
}

void *malloc_psram(uint32 size)                   __alias(_os_malloc_psram);
void  free_psram(void *ptr)                       __alias(_os_free_psram);
void *zalloc_psram(size_t size)                   __alias(_os_zalloc_psram);
void *calloc_psram(size_t nmemb, size_t size)     __alias(_os_calloc_psram);
void *realloc_psram(void *ptr, size_t size)       __alias(_os_realloc_psram);
#else
void *_os_malloc_psram(int size)                  __alias(_os_malloc);
void  _os_free_psram(void *ptr)                   __alias(_os_free);
void *_os_zalloc_psram(size_t size)               __alias(_os_zalloc);
void *_os_calloc_psram(size_t nmemb, size_t size) __alias(_os_calloc);
void *_os_realloc_psram(void *ptr, int size)      __alias(_os_realloc);

void *_os_malloc_psram_t(int size, const char *func, int line)                  __alias(_os_malloc_t);
void  _os_free_psram_t(void *ptr, const char *func, int line)                   __alias(_os_free_t);
void *_os_zalloc_psram_t(int size, const char *func, int line)                  __alias(_os_zalloc_t);
void *_os_calloc_psram_t(size_t nmemb, size_t size, const char *func, int line) __alias(_os_calloc_t);
void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line)      __alias(_os_realloc_t);
#endif
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// 在这里可以 继续添加 自定义heap 的 API.


#endif

#ifdef MEM_FREE_REC
struct mem_freerec{
    uint32 ptr;
    uint32 lr;
};
RBUFFER_DEF(g_freerec, struct mem_freerec, MEM_FREE_REC);

void mem_free_rec(uint32 ptr, uint32 lr)
{
    struct mem_freerec rec = {ptr, lr};
    if (g_freerec.qsize == 0) {
        RB_INIT(&g_freerec, MEM_FREE_REC);
    }
    RB_INT_SET_F(&g_freerec, rec);
}

void mem_free_dump(void)
{
    uint32 i = 0;
    uint32 flag = disable_irq();
    os_printf("MEM FREE record list:\r\n");
    for (i = 0; i < g_freerec.qsize; i++) {
        if (g_freerec.rbq[i].ptr) {
            os_printf("     addr:%p, LR:%p\r\n", g_freerec.rbq[i].ptr, g_freerec.rbq[i].lr);
        }
    }
    enable_irq(flag);
}

#endif
