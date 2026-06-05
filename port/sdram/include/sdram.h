#ifndef BSP_SDRAM_H_
#define BSP_SDRAM_H_

#include "stm32h7xx_hal.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// SDRAM 参数宏
//--------------------------------------------------------------------+
#define SDRAM_SIZE            (32 * 1024 * 1024)
#define SDRAM_BANK_ADDR       ((uint32_t)0xC0000000)
#define FMC_COMMAND_TARGET_BANK  FMC_SDRAM_CMD_TARGET_BANK1
#define SDRAM_TIMEOUT         ((uint32_t)0x1000)

//--------------------------------------------------------------------+
// SDRAM Mode 寄存器位定义
//--------------------------------------------------------------------+
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void sdram_init_sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *command);

//--------------------------------------------------------------------+
// 单元测试 (USE_BSP_UNIT_TESTING 宏控制)
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void sdram_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
