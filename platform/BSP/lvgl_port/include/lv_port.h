#ifndef BSP_LV_PORT_H_
#define BSP_LV_PORT_H_

#include <stdint.h>
#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

// 初始化 LVGL (显示 + DMA2D + tick)
void lv_port_init(void);

#if !BSP_FREERTOS_ENABLED
// LVGL 自定义 tick 源 (LV_TICK_CUSTOM_SYS_TIME_EXPR), 仅裸机模式
uint32_t lv_port_tick_get(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
