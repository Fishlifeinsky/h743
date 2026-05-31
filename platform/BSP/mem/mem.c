#include "mem.h"
#include <string.h>

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void mem_itcm_load(void) {
    size_t size = (size_t)(&__itcm_end__ - &__itcm_start__);
    if (size == 0) return;
    memcpy(&__itcm_start__, &__itcm_load__, size);
}

void mem_sdram_load(void) {
    size_t size = (size_t)(&__sdram_code_end__ - &__sdram_code_start__);
    if (size == 0) return;
    memcpy(&__sdram_code_start__, &__sdram_code_load__, size);
}

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static MEM_ITCM_SECTION int mem_itcm_test_add(int a, int b) {
    return a + b;
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void mem_itcm_test_loaded(int a, int b) {
    DEBUG_PRINT("\r\n======== ITCM loaded test ========\r\n");
    DEBUG_PRINT("  -> function addr: %p (should be 0x0000xxxx in ITCM)\r\n",
           mem_itcm_test_add);
    int result = mem_itcm_test_add(a, b);
    DEBUG_PRINT("  -> %d + %d = %d\r\n", a, b, result);
    DEBUG_PRINT("  [PASS] ITCM function executed OK after load\r\n");
}

void mem_itcm_test_noload(int a, int b) {
    DEBUG_PRINT("\r\n======== ITCM noload test ========\r\n");
    DEBUG_PRINT("  -> calling mem_itcm_test_add() WITHOUT load\r\n");
    DEBUG_PRINT("  -> ITCM RAM is empty, this will HardFault!\r\n");
    DEBUG_PRINT("  -> about to crash...\r\n\r\n");
    mem_itcm_test_add(a, b);
}

#endif // USE_BSP_UNIT_TESTING
