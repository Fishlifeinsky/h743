/**
  * @file    touch.h
  * @brief   GT911 触摸控制器驱动 (800x480 RGB LCD)
  *
  *          I2C 地址: 0xBA(写) / 0xBB(读)
  *          最多 5 点触摸
  */

#ifndef __TOUCH_800x480_H
#define __TOUCH_800x480_H

#include "stm32h7xx_hal.h"
#include "touch_iic.h"

#define TOUCH_MAX   5

typedef struct {
    uint8_t  flag;            /* 触摸标志: 1=有触摸 */
    uint8_t  num;             /* 触摸点数 */
    uint16_t x[TOUCH_MAX];    /* X 坐标 */
    uint16_t y[TOUCH_MAX];    /* Y 坐标 */
} TouchStructure;

extern volatile TouchStructure touchInfo;

/* GT911 寄存器 */
#define GT9XX_IIC_RADDR  0xBB   /* I2C 读地址 */
#define GT9XX_IIC_WADDR  0xBA   /* I2C 写地址 */

#define GT9XX_CFG_ADDR   0x8047 /* 配置寄存器起始地址 */
#define GT9XX_READ_ADDR  0x814E /* 触摸信息寄存器 */
#define GT9XX_ID_ADDR    0x8140 /* 产品 ID 寄存器 */

/* 接口函数 */
uint8_t  Touch_Init(void);
void     Touch_Scan(void);
void     GT9XX_Reset(void);
void     GT9XX_SendCfg(void);
void     GT9XX_ReadCfg(void);

#endif
