#ifndef BSP_LV_PORT_H_
#define BSP_LV_PORT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

// 初始化 LVGL (显示 + DMA2D + tick)
void lv_port_init(void);

// LVGL 自定义 tick 源 (LV_TICK_CUSTOM_SYS_TIME_EXPR)
uint32_t lv_port_tick_get(void);

#ifdef __cplusplus
}
#endif

#endif
