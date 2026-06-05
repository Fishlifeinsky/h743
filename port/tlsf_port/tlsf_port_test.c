#include "tlsf_port.h"
#include <string.h>
#include <stdint.h>

#if USE_BSP_UNIT_TESTING

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

// 单池分配/释放/重分配测试
static int test_pool_alloc(tlsf_port_pool_t pool, const char *name) {
    int err = 0;

    void *p1 = tlsf_port_malloc(128, pool);
    if (!p1) {
        DEBUG_PRINT("  %s: malloc(128) failed\r\n", name);
        return 1;
    }
    memset(p1, 0xAB, 128);

    void *p2 = tlsf_port_malloc(256, pool);
    if (!p2) {
        DEBUG_PRINT("  %s: malloc(256) failed\r\n", name);
        tlsf_port_free(p1);
        return 1;
    }
    memset(p2, 0xCD, 256);

    // p1 和 p2 不能重叠
    if ((uint8_t *)p1 + 128 > (uint8_t *)p2 &&
        (uint8_t *)p2 + 256 > (uint8_t *)p1) {
        DEBUG_PRINT("  %s: overlap detected\r\n", name);
        err++;
    }

    // 重分配 p1 到更大
    void *p1r = tlsf_port_realloc(p1, 200);
    if (!p1r) {
        DEBUG_PRINT("  %s: realloc failed\r\n", name);
        err++;
    } else {
        // 原有数据 0xAB 应保留
        uint8_t ref[128];
        memset(ref, 0xAB, 128);
        if (memcmp(p1r, ref, 128) != 0) {
            DEBUG_PRINT("  %s: realloc data mismatch\r\n", name);
            err++;
        }
        p1 = p1r;
    }

    tlsf_port_free(p1);
    tlsf_port_free(p2);

    // 释放后应能重新分配
    void *p3 = tlsf_port_malloc(128, pool);
    if (!p3) {
        DEBUG_PRINT("  %s: re-malloc after free failed\r\n", name);
        err++;
    } else {
        tlsf_port_free(p3);
    }

    DEBUG_PRINT("  %s: %s\r\n", name, err ? "FAIL" : "PASS");
    return err;
}

// 测试 free 自动识别池 (SDRAM 单池版本)
static int test_free_auto_detect(void) {
    int err = 0;

    // 从 SDRAM 分配一块
    void *ps = tlsf_port_malloc(64, TLSF_PORT_SDRAM);
    if (!ps) {
        DEBUG_PRINT("  auto-detect: alloc failed\r\n");
        return 1;
    }

    // 不需要指定池就能 free
    tlsf_port_free(ps);

    // 释放后应能重新分配
    void *ps2 = tlsf_port_malloc(64, TLSF_PORT_SDRAM);
    if (!ps2) {
        DEBUG_PRINT("  auto-detect: re-alloc failed\r\n");
        err++;
    }
    tlsf_port_free(ps2);

    // 释放 NULL 不应崩溃
    tlsf_port_free(NULL);

    DEBUG_PRINT("  auto-detect: %s\r\n", err ? "FAIL" : "PASS");
    return err;
}

// 非法 realloc 参数测试
static int test_realloc_edge(void) {
    void *p = tlsf_port_realloc(NULL, 10);
    if (p) {
        DEBUG_PRINT("  null-realloc: expected NULL, got %p\r\n", p);
        tlsf_port_free(p);
        return 1;
    }
    DEBUG_PRINT("  null-realloc: PASS\r\n");
    return 0;
}

//--------------------------------------------------------------------+
// 单元测试入口
//--------------------------------------------------------------------+

void tlsf_port_test(void) {
    DEBUG_PRINT("\r\n=== TLSF Port Test ===\r\n");

    int errors = 0;

    // SDRAM 池
    errors += test_pool_alloc(TLSF_PORT_SDRAM, "SDRAM");

    // SRAM D1 池
    tlsf_port_pool_t d1_pool = TLSF_PORT_SRAM_D1;
    if (tlsf_port_get(d1_pool)) {
        errors += test_pool_alloc(d1_pool, "SRAM_D1");
    } else {
        DEBUG_PRINT("  SRAM D1: SKIP (pool not created)\r\n");
    }

    // free 自动识别
    errors += test_free_auto_detect();

    // 非法指针
    errors += test_realloc_edge();

    DEBUG_PRINT("=== TLSF Port Test: %d error(s) ===\r\n", errors);
}

#endif /* USE_BSP_UNIT_TESTING */
