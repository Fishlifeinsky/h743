#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "tlsf_port.h"

void *__wrap_malloc(size_t size)
{
    return tlsf_port_malloc(size, BSP_MALLOC_DEFAULT_POOL);
}

void __wrap_free(void *ptr)
{
    tlsf_port_free(ptr);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    return tlsf_port_realloc(ptr, size);
}

void *__wrap_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *ptr = tlsf_port_malloc(total, BSP_MALLOC_DEFAULT_POOL);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

#if USE_BSP_UNIT_TESTING

void malloc_wrap_test(void)
{
    DEBUG_PRINT("\r\n=== Malloc Wrap Test ===\r\n");
    int err = 0;

    // 打印内存边界，确认走的是 TLSF 而不是 newlib heap
    extern uint8_t _end;
    extern uint8_t __sram_d1_free_start__;
    extern uint8_t _estack;
    extern uint32_t _Min_Stack_Size;

    DEBUG_PRINT("  newlib  heap: [%p, %p)\r\n",
                &_end, (uint8_t *)&_estack - _Min_Stack_Size);
    DEBUG_PRINT("  TLSF SRAM_D1: [%p, %p)\r\n",
                &__sram_d1_free_start__,
                (uint8_t *)0x24000000 + 512 * 1024);

    // 1) malloc → write → free
    char *p1 = malloc(128);
    if (!p1) {
        DEBUG_PRINT("  malloc(128): FAIL (NULL)\r\n");
        err++;
    } else {
        memset(p1, 0x5A, 128);
        DEBUG_PRINT("  malloc(128): ok p=%p\r\n", p1);
    }

    // 2) calloc → should be zeroed
    int *p2 = calloc(16, sizeof(int));
    if (!p2) {
        DEBUG_PRINT("  calloc(16*4): FAIL (NULL)\r\n");
        err++;
    } else {
        int zero = 1;
        for (int i = 0; i < 16; i++) {
            if (p2[i] != 0) { zero = 0; break; }
        }
        if (!zero) {
            DEBUG_PRINT("  calloc: FAIL (not zeroed)\r\n");
            err++;
        } else {
            DEBUG_PRINT("  calloc(16*4): ok p=%p (zeroed)\r\n", p2);
        }
    }

    // 3) realloc → enlarge, check data preserved
    if (p1) {
        char *p1r = realloc(p1, 256);
        if (!p1r) {
            DEBUG_PRINT("  realloc(256): FAIL (NULL)\r\n");
            err++;
        } else {
            uint8_t ref[128];
            memset(ref, 0x5A, 128);
            if (memcmp(p1r, ref, 128) != 0) {
                DEBUG_PRINT("  realloc: FAIL (data mismatch)\r\n");
                err++;
            } else {
                DEBUG_PRINT("  realloc(256): ok p=%p (data preserved)\r\n", p1r);
            }
            p1 = p1r;
        }
    }

    // 4) free → re-alloc should reuse or succeed
    free(p1);
    free(p2);
    void *p3 = malloc(64);
    if (!p3) {
        DEBUG_PRINT("  malloc after free: FAIL\r\n");
        err++;
    } else {
        DEBUG_PRINT("  malloc(64) after free: ok p=%p\r\n", p3);
        free(p3);
    }

    // 5) free(NULL) should not crash
    free(NULL);
    DEBUG_PRINT("  free(NULL): ok (no crash)\r\n");

    DEBUG_PRINT("=== Malloc Wrap Test: %d error(s) ===\r\n", err);
}

#endif
