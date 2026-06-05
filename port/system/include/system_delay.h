#ifndef BSP_SYSTEM_DELAY_H_
#define BSP_SYSTEM_DELAY_H_

#include "port/config.h"

#include "stm32h7xx_hal.h"
#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 延时函数 (inline)
//   FreeRTOS 环境 → vTaskDelay (阻塞任务, 释放 CPU)
//   裸机环境      → HAL_Delay (忙等待)
//--------------------------------------------------------------------+

static inline void system_delay_ms(uint32_t ms)
{
#if BSP_FREERTOS_ENABLED
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        HAL_Delay(ms);
    }
#else
    HAL_Delay(ms);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_SYSTEM_DELAY_H_ */
