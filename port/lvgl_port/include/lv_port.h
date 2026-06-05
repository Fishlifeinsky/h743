/**
  * @file    lv_port.h
  * @brief   LVGL v9 移植头文件 (基于 lv_port_disp_template.h)
  */

#ifndef BSP_LV_PORT_H_
#define BSP_LV_PORT_H_

#include <stdint.h>
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_port_init(void);

#if !BSP_FREERTOS_ENABLED
uint32_t lv_port_tick_get(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
