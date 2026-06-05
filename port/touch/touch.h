/**
  * @file    touch.h
  * @brief   GT911 触摸控制器驱动 (800x480 RGB LCD)
  *
  *          I2C 地址: 0xBA(写) / 0xBB(读), 最多 5 点触摸
  */

#ifndef __TOUCH_H__
#define __TOUCH_H__

#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "touch_iic.h"

#define TOUCH_MAX_POINTS     5

#define TOUCH_GT9XX_IIC_RADDR  0xBB
#define TOUCH_GT9XX_IIC_WADDR  0xBA
#define TOUCH_GT9XX_CFG_ADDR   0x8047
#define TOUCH_GT9XX_READ_ADDR  0x814E
#define TOUCH_GT9XX_ID_ADDR    0x8140

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

uint8_t touch_init(void);
void    touch_scan(void);
bool    touch_is_touch(void);
uint8_t touch_get_num(void);
void    touch_read_xy(uint8_t idx, uint16_t *x, uint16_t *y);

#endif
