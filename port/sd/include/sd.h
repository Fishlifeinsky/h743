#ifndef BSP_SD_H_
#define BSP_SD_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// SD 卡 API — 所有 SD 卡操作封装, 内置互斥锁保护 (FreeRTOS 环境下)
//--------------------------------------------------------------------+

int  sd_init(void);         // 初始化 SD 卡硬件 (4-bit 宽总线 + 高速模式)
void sd_rtos_init(void);     // 创建 RTOS 互斥锁 (调度器启动后调用)

int  sd_read(uint8_t *buf, uint32_t sector, uint32_t count, uint32_t timeout_ms);
int  sd_write(const uint8_t *buf, uint32_t sector, uint32_t count, uint32_t timeout_ms);
int  sd_erase(uint32_t sector_start, uint32_t sector_end);
int  sd_get_info(HAL_SD_CardInfoTypeDef *info);
int  sd_get_state(HAL_SD_CardStateTypeDef *state);
int  sd_is_ready(void);

//--------------------------------------------------------------------+
// 单元测试 (USE_BSP_UNIT_TESTING 宏控制)
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void sd_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
