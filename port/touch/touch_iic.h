/**
  * @file    touch_iic.h
  * @brief   GT911 触摸 I2C 底层 (软件模拟 I2C)
  *
  *          SCL=PI11, SDA=PI8, INT=PG3, RST=PH4
  *          通讯速率 ~100KHz
  */

#ifndef __TOUCH_IIC_H
#define __TOUCH_IIC_H

#include "stm32h7xx_hal.h"

/* I2C 引脚配置 */
#define Touch_IIC_SCL_CLK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define Touch_IIC_SCL_PORT           GPIOI
#define Touch_IIC_SCL_PIN            GPIO_PIN_11

#define Touch_IIC_SDA_CLK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define Touch_IIC_SDA_PORT           GPIOI
#define Touch_IIC_SDA_PIN            GPIO_PIN_8

#define Touch_INT_CLK_ENABLE         __HAL_RCC_GPIOG_CLK_ENABLE()
#define Touch_INT_PORT               GPIOG
#define Touch_INT_PIN                GPIO_PIN_3

#define Touch_RST_CLK_ENABLE         __HAL_RCC_GPIOH_CLK_ENABLE()
#define Touch_RST_PORT               GPIOH
#define Touch_RST_PIN                GPIO_PIN_4

/* I2C ACK 状态 */
#define ACK_OK   1
#define ACK_ERR  0

/* I2C 延时参数 (通讯速率 ~100KHz) */
#define IIC_DelayVaule  20

/* IO 操作宏 */
#define Touch_IIC_SCL(a)  if (a) HAL_GPIO_WritePin(Touch_IIC_SCL_PORT, Touch_IIC_SCL_PIN, GPIO_PIN_SET); \
                          else   HAL_GPIO_WritePin(Touch_IIC_SCL_PORT, Touch_IIC_SCL_PIN, GPIO_PIN_RESET)

#define Touch_IIC_SDA(a)  if (a) HAL_GPIO_WritePin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN, GPIO_PIN_SET); \
                          else   HAL_GPIO_WritePin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN, GPIO_PIN_RESET)

/* 接口函数 */
void     Touch_IIC_GPIO_Config(void);
void     Touch_IIC_Delay(uint32_t a);
void     Touch_INT_Out(void);
void     Touch_INT_In(void);
void     Touch_IIC_Start(void);
void     Touch_IIC_Stop(void);
void     Touch_IIC_ACK(void);
void     Touch_IIC_NoACK(void);
uint8_t  Touch_IIC_WaitACK(void);
uint8_t  Touch_IIC_WriteByte(uint8_t IIC_Data);
uint8_t  Touch_IIC_ReadByte(uint8_t ACK_Mode);

#endif
