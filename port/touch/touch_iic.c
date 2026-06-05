/**
  * @file    touch_iic.c
  * @brief   GT911 触摸 I2C 底层驱动 (软件模拟 I2C, ~100KHz)
  */

#include "touch_iic.h"

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void touch_iic_gpio_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    TOUCH_IIC_SCL_CLK_ENABLE;
    TOUCH_IIC_SDA_CLK_ENABLE;
    TOUCH_IIC_INT_CLK_ENABLE;
    TOUCH_IIC_RST_CLK_ENABLE;

    /* SCL + SDA: 开漏输出 */
    GPIO_InitStruct.Pin   = TOUCH_IIC_SCL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TOUCH_IIC_SCL_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = TOUCH_IIC_SDA_PIN;
    HAL_GPIO_Init(TOUCH_IIC_SDA_PORT, &GPIO_InitStruct);

    /* INT + RST: 推挽输出 + 上拉 */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin   = TOUCH_IIC_INT_PIN;
    HAL_GPIO_Init(TOUCH_IIC_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = TOUCH_IIC_RST_PIN;
    HAL_GPIO_Init(TOUCH_IIC_RST_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(TOUCH_IIC_SCL_PORT, TOUCH_IIC_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_IIC_SDA_PORT, TOUCH_IIC_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_IIC_INT_PORT, TOUCH_IIC_INT_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOUCH_IIC_RST_PORT, TOUCH_IIC_RST_PIN, GPIO_PIN_SET);
}

void touch_iic_delay(uint32_t a)
{
    volatile uint16_t i;
    while (a--) {
        for (i = 0; i < 8; i++);
    }
}

void touch_iic_int_out(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin   = TOUCH_IIC_INT_PIN;

    HAL_GPIO_Init(TOUCH_IIC_INT_PORT, &GPIO_InitStruct);
}

void touch_iic_int_in(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin   = TOUCH_IIC_INT_PIN;

    HAL_GPIO_Init(TOUCH_IIC_INT_PORT, &GPIO_InitStruct);
}

void touch_iic_start(void)
{
    TOUCH_IIC_SDA(1);
    TOUCH_IIC_SCL(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);

    TOUCH_IIC_SDA(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SCL(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
}

void touch_iic_stop(void)
{
    TOUCH_IIC_SCL(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SDA(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);

    TOUCH_IIC_SCL(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SDA(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
}

void touch_iic_ack(void)
{
    TOUCH_IIC_SCL(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SDA(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SCL(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);

    TOUCH_IIC_SCL(0);
    TOUCH_IIC_SDA(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
}

void touch_iic_noack(void)
{
    TOUCH_IIC_SCL(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SDA(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SCL(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);

    TOUCH_IIC_SCL(0);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
}

uint8_t touch_iic_wait_ack(void)
{
    TOUCH_IIC_SDA(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    TOUCH_IIC_SCL(1);
    touch_iic_delay(TOUCH_IIC_DELAY_VAL);

    if (HAL_GPIO_ReadPin(TOUCH_IIC_SDA_PORT, TOUCH_IIC_SDA_PIN) != 0) {
        TOUCH_IIC_SCL(0);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
        return TOUCH_IIC_ACK_ERR;
    } else {
        TOUCH_IIC_SCL(0);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
        return TOUCH_IIC_ACK_OK;
    }
}

uint8_t touch_iic_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        TOUCH_IIC_SDA(data & 0x80);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
        TOUCH_IIC_SCL(1);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
        TOUCH_IIC_SCL(0);
        if (i == 7) TOUCH_IIC_SDA(1);
        data <<= 1;
    }
    return touch_iic_wait_ack();
}

uint8_t touch_iic_read_byte(uint8_t ack_mode)
{
    uint8_t data = 0;

    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        TOUCH_IIC_SCL(1);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
        data |= (HAL_GPIO_ReadPin(TOUCH_IIC_SDA_PORT, TOUCH_IIC_SDA_PIN) & 0x01);
        TOUCH_IIC_SCL(0);
        touch_iic_delay(TOUCH_IIC_DELAY_VAL);
    }

    if (ack_mode == 1)
        touch_iic_ack();
    else
        touch_iic_noack();

    return data;
}
