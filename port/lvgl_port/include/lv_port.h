/**
  * @file    lv_port.h
  * @brief   LVGL v9 移植 (显示 + 触摸输入)
  */

#ifndef __LV_PORT_H__
#define __LV_PORT_H__

#include <stdint.h>
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void     lv_port_init(void);
void     lv_port_disp_init(void);
void     lv_port_indev_init(void);

#if !BSP_FREERTOS_ENABLED
uint32_t lv_port_tick_get(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
