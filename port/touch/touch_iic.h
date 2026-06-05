/**
  * @file    touch_iic.h
  * @brief   GT911 触摸 I2C 底层 (软件模拟 I2C)
  *
  *          SCL=PI11, SDA=PI8, INT=PG3, RST=PH4
  *          通讯速率 ~100KHz
  */

#ifndef __TOUCH_IIC_H__
#define __TOUCH_IIC_H__

#include "stm32h7xx_hal.h"

/* I2C 引脚 */
#define TOUCH_IIC_SCL_CLK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define TOUCH_IIC_SCL_PORT           GPIOI
#define TOUCH_IIC_SCL_PIN            GPIO_PIN_11

#define TOUCH_IIC_SDA_CLK_ENABLE     __HAL_RCC_GPIOI_CLK_ENABLE()
#define TOUCH_IIC_SDA_PORT           GPIOI
#define TOUCH_IIC_SDA_PIN            GPIO_PIN_8

#define TOUCH_IIC_INT_CLK_ENABLE     __HAL_RCC_GPIOG_CLK_ENABLE()
#define TOUCH_IIC_INT_PORT           GPIOG
#define TOUCH_IIC_INT_PIN            GPIO_PIN_3

#define TOUCH_IIC_RST_CLK_ENABLE     __HAL_RCC_GPIOH_CLK_ENABLE()
#define TOUCH_IIC_RST_PORT           GPIOH
#define TOUCH_IIC_RST_PIN            GPIO_PIN_4

/* I2C ACK 状态 */
#define TOUCH_IIC_ACK_OK    1
#define TOUCH_IIC_ACK_ERR   0

/* I2C 延时参数 (通讯速率 ~100KHz) */
#define TOUCH_IIC_DELAY_VAL  20

/* IO 操作宏 */
#define TOUCH_IIC_SCL(a)  if (a) HAL_GPIO_WritePin(TOUCH_IIC_SCL_PORT, TOUCH_IIC_SCL_PIN, GPIO_PIN_SET); \
                          else   HAL_GPIO_WritePin(TOUCH_IIC_SCL_PORT, TOUCH_IIC_SCL_PIN, GPIO_PIN_RESET)

#define TOUCH_IIC_SDA(a)  if (a) HAL_GPIO_WritePin(TOUCH_IIC_SDA_PORT, TOUCH_IIC_SDA_PIN, GPIO_PIN_SET); \
                          else   HAL_GPIO_WritePin(TOUCH_IIC_SDA_PORT, TOUCH_IIC_SDA_PIN, GPIO_PIN_RESET)

//--------------------------------------------------------------------+
// 接口函数
//--------------------------------------------------------------------+

void     touch_iic_gpio_config(void);
void     touch_iic_delay(uint32_t a);
void     touch_iic_int_out(void);
void     touch_iic_int_in(void);
void     touch_iic_start(void);
void     touch_iic_stop(void);
void     touch_iic_ack(void);
void     touch_iic_noack(void);
uint8_t  touch_iic_wait_ack(void);
uint8_t  touch_iic_write_byte(uint8_t data);
uint8_t  touch_iic_read_byte(uint8_t ack_mode);

#endif
