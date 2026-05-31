#ifndef BSP_MEM_H_
#define BSP_MEM_H_

#include <stdint.h>
#include <stddef.h>
#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 内存段属性宏，将变量放入对应内存区域
//--------------------------------------------------------------------+
#define MEM_SRAM_D1_SECTION     __attribute__((section(".sram_d1")))
#define MEM_SRAM_D2_DMA_SECTION __attribute__((section(".sram_d2_dma")))
#define MEM_SRAM_D2_USB_SECTION __attribute__((section(".sram_d2_usb")))
#define MEM_SRAM_DTCM_SECTION   __attribute__((section(".sram_dtcm")))
#define MEM_SRAM_D3_SECTION     __attribute__((section(".sram_d3")))
#define MEM_SRAM_BKP_SECTION    __attribute__((section(".sram_bkp")))
#define MEM_ITCM_SECTION        __attribute__((section(".itcm_ram_func")))
#define MEM_SDRAM_CODE_SECTION  __attribute__((section(".sdram_code")))
#define MEM_SDRAM_DATA_SECTION  __attribute__((section(".sdram_data")))

//--------------------------------------------------------------------+
// SRAM_D1: 域 1 总线矩阵 (0x24000000, 512KB)
//   .data / .bss / heap / stack 分配之后，剩余空间为 .sram_d1
//--------------------------------------------------------------------+
#define MEM_SRAM_D1_START       ((uint32_t)0x24000000)
#define MEM_SRAM_D1_TOTAL_SIZE  (512 * 1024)

extern uint8_t __sram_d1_free_start__;

#define MEM_SRAM_D1_FREE_START  ((uint32_t)&__sram_d1_free_start__)
#define MEM_SRAM_D1_FREE_SIZE   (MEM_SRAM_D1_START + MEM_SRAM_D1_TOTAL_SIZE - MEM_SRAM_D1_FREE_START)

//--------------------------------------------------------------------+
// SRAM_D2_DMA: SRAM1+SRAM2 (0x30000000, 256KB) — DMA 缓冲区
//--------------------------------------------------------------------+
#define MEM_SRAM_D2_DMA_START       ((uint32_t)0x30000000)
#define MEM_SRAM_D2_DMA_TOTAL_SIZE  (256 * 1024)

extern uint8_t __sram_d2_dma_start__;
extern uint8_t __sram_d2_dma_free_start__;

#define MEM_SRAM_D2_DMA_FREE_START  ((uint32_t)&__sram_d2_dma_free_start__)
#define MEM_SRAM_D2_DMA_FREE_SIZE   (MEM_SRAM_D2_DMA_START + MEM_SRAM_D2_DMA_TOTAL_SIZE - MEM_SRAM_D2_DMA_FREE_START)

//--------------------------------------------------------------------+
// SRAM_D2_USB: SRAM3 (0x30040000, 32KB) — USB/ETH 缓冲区
//--------------------------------------------------------------------+
#define MEM_SRAM_D2_USB_START       ((uint32_t)0x30040000)
#define MEM_SRAM_D2_USB_TOTAL_SIZE  (32 * 1024)

extern uint8_t __sram_d2_usb_start__;
extern uint8_t __sram_d2_usb_end__;

//--------------------------------------------------------------------+
// SRAM_DTCM: Data TCM (0x20000000, 128KB)
//   CPU 直接访问，0 wait，不经 Cache，DMA 不能直接访问（MDMA 除外）
//--------------------------------------------------------------------+
#define MEM_SRAM_DTCM_START       ((uint32_t)0x20000000)
#define MEM_SRAM_DTCM_TOTAL_SIZE  (128 * 1024)

extern uint8_t __sram_dtcm_start__;
extern uint8_t __sram_dtcm_end__;

//--------------------------------------------------------------------+
// SRAM_D3: 域 3 总线矩阵 (0x38000000, 64KB)
//   低功耗保留，BDMA 可访问
//--------------------------------------------------------------------+
#define MEM_SRAM_D3_START       ((uint32_t)0x38000000)
#define MEM_SRAM_D3_TOTAL_SIZE  (64 * 1024)

extern uint8_t __sram_d3_start__;
extern uint8_t __sram_d3_end__;

//--------------------------------------------------------------------+
// Backup SRAM (0x38800000, 4KB)
//   VBAT 供电可保持
//--------------------------------------------------------------------+
#define MEM_SRAM_BKP_START       ((uint32_t)0x38800000)
#define MEM_SRAM_BKP_TOTAL_SIZE  (4 * 1024)

extern uint8_t __sram_bkp_start__;
extern uint8_t __sram_bkp_end__;

//--------------------------------------------------------------------+
// ITCM: Instruction TCM (0x00000000, 64KB)
//   代码执行专用，FLASH 存放，启动时由 mem_itcm_load() 搬运
//--------------------------------------------------------------------+
#define MEM_ITCM_START       ((uint32_t)0x00000000)
#define MEM_ITCM_TOTAL_SIZE  (64 * 1024)

extern uint8_t __itcm_start__;
extern uint8_t __itcm_end__;
extern uint8_t __itcm_load__;

//--------------------------------------------------------------------+
// SDRAM: 外部 SDRAM 执行代码 (0xC0000000, 32MB)
//   FLASH 存放，启动时由 mem_sdram_load() 搬运
//   .sdram_code 之后剩余空间供 TLSF 管理
//--------------------------------------------------------------------+
#define MEM_SDRAM_START       ((uint32_t)0xC0000000)
#define MEM_SDRAM_TOTAL_SIZE  (32 * 1024 * 1024)

extern uint8_t __sdram_code_start__;
extern uint8_t __sdram_code_end__;
extern uint8_t __sdram_code_load__;
extern uint8_t __sdram_free_start__;

#define MEM_SDRAM_CODE_START  ((uint32_t)&__sdram_code_start__)
#define MEM_SDRAM_CODE_END    ((uint32_t)&__sdram_code_end__)
#define MEM_SDRAM_FREE_START  ((uint32_t)&__sdram_free_start__)
#define MEM_SDRAM_FREE_SIZE   (MEM_SDRAM_START + MEM_SDRAM_TOTAL_SIZE - MEM_SDRAM_FREE_START)

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void mem_itcm_load(void);
void mem_sdram_load(void);

//--------------------------------------------------------------------+
// 单元测试 (USE_BSP_UNIT_TESTING 宏控制)
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void mem_itcm_test_loaded(int a, int b);
void mem_itcm_test_noload(int a, int b);
void malloc_wrap_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
