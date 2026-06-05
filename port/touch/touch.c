/**
  * @file    touch.c
  * @brief   GT911 触摸控制器驱动 (800x480 RGB LCD)
  */

#include "touch.h"
#include "port/config.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "semphr.h"
#endif

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static touch_data_t g_touch_data;
static volatile uint8_t g_modify_flag = 0;

#if BSP_FREERTOS_ENABLED
static SemaphoreHandle_t g_mutex;
static StaticSemaphore_t g_mutex_buf;
#endif

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static void gt9xx_reset(void);
static uint8_t gt9xx_write_handle(uint16_t addr);
static uint8_t gt9xx_write_data(uint16_t addr, uint8_t value);
static uint8_t gt9xx_write_reg(uint16_t addr, uint8_t cnt, uint8_t *value);
static uint8_t gt9xx_read_reg(uint16_t addr, uint8_t cnt, uint8_t *value);
static void panel_recognition(void);

//--------------------------------------------------------------------+
// 互斥锁
//--------------------------------------------------------------------+

#if BSP_FREERTOS_ENABLED
#define TOUCH_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
#define TOUCH_UNLOCK() do { if (g_mutex) xSemaphoreGive(g_mutex); } while(0)
#else
#define TOUCH_LOCK()   ((void)0)
#define TOUCH_UNLOCK() ((void)0)
#endif

//--------------------------------------------------------------------+
// 复位 GT911
//--------------------------------------------------------------------+

static void gt9xx_reset(void)
{
    touch_iic_int_out();

    HAL_GPIO_WritePin(TOUCH_IIC_INT_PORT, TOUCH_IIC_INT_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOUCH_IIC_RST_PORT, TOUCH_IIC_RST_PIN, GPIO_PIN_SET);
    touch_iic_delay(10000);

    HAL_GPIO_WritePin(TOUCH_IIC_RST_PORT, TOUCH_IIC_RST_PIN, GPIO_PIN_RESET);
    touch_iic_delay(150000);
    HAL_GPIO_WritePin(TOUCH_IIC_RST_PORT, TOUCH_IIC_RST_PIN, GPIO_PIN_SET);
    touch_iic_delay(350000);
    touch_iic_int_in();
    touch_iic_delay(20000);
}

//--------------------------------------------------------------------+
// I2C 读写底层
//--------------------------------------------------------------------+

static uint8_t gt9xx_write_handle(uint16_t addr)
{
    touch_iic_start();
    if (touch_iic_write_byte(TOUCH_GT9XX_IIC_WADDR) != TOUCH_IIC_ACK_OK) {
        touch_iic_stop();
        return ERROR;
    }
    touch_iic_write_byte((uint8_t)(addr >> 8));
    touch_iic_write_byte((uint8_t)(addr));
    return SUCCESS;
}

static uint8_t gt9xx_write_data(uint16_t addr, uint8_t value)
{
    touch_iic_start();
    if (gt9xx_write_handle(addr) != SUCCESS) {
        touch_iic_stop();
        return ERROR;
    }
    touch_iic_write_byte(value);
    touch_iic_stop();
    return SUCCESS;
}

static uint8_t gt9xx_write_reg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
    touch_iic_start();
    if (gt9xx_write_handle(addr) != SUCCESS) {
        touch_iic_stop();
        return ERROR;
    }
    for (uint8_t i = 0; i < cnt; i++)
        touch_iic_write_byte(value[i]);
    touch_iic_stop();
    return SUCCESS;
}

static uint8_t gt9xx_read_reg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
    touch_iic_start();
    if (gt9xx_write_handle(addr) != SUCCESS) {
        touch_iic_stop();
        return ERROR;
    }

    touch_iic_start();
    if (touch_iic_write_byte(TOUCH_GT9XX_IIC_RADDR) != TOUCH_IIC_ACK_OK) {
        touch_iic_stop();
        return ERROR;
    }

    for (uint8_t i = 0; i < cnt; i++) {
        if (i == (cnt - 1))
            value[i] = touch_iic_read_byte(0);
        else
            value[i] = touch_iic_read_byte(1);
    }
    touch_iic_stop();
    return SUCCESS;
}

//--------------------------------------------------------------------+
// 硬件版本识别
//--------------------------------------------------------------------+

static void panel_recognition(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    TOUCH_IIC_INT_CLK_ENABLE;
    TOUCH_IIC_RST_CLK_ENABLE;

    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin   = TOUCH_IIC_INT_PIN;
    HAL_GPIO_Init(TOUCH_IIC_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = TOUCH_IIC_RST_PIN;
    HAL_GPIO_Init(TOUCH_IIC_RST_PORT, &GPIO_InitStruct);

    touch_iic_delay(4000);

    if ((HAL_GPIO_ReadPin(TOUCH_IIC_RST_PORT, TOUCH_IIC_RST_PIN) != 1) &&
        (HAL_GPIO_ReadPin(TOUCH_IIC_INT_PORT, TOUCH_IIC_INT_PIN) != 1)) {
        g_modify_flag = 1;
    }
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void touch_rtos_init(void)
{
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buf);
#endif
}

uint8_t touch_init(void)
{
    uint8_t info[11];
    uint8_t cfg_version = 0;

    panel_recognition();
    touch_iic_gpio_config();
    gt9xx_reset();

    gt9xx_read_reg(TOUCH_GT9XX_ID_ADDR, 11, info);
    gt9xx_read_reg(TOUCH_GT9XX_CFG_ADDR, 1, &cfg_version);

    if (info[0] != '9') {
        DEBUG_PRINT("touch: no GT911 detected\r\n");
        return ERROR;
    }

    DEBUG_PRINT("touch: ID GT%.4s, fw 0x%04x, res %dx%d, cfg 0x%02x\r\n",
                info,
                (info[5] << 8) + info[4],
                (info[7] << 8) + info[6],
                (info[9] << 8) + info[8],
                cfg_version);

    uint16_t res_x = (info[7] << 8) + info[6];
    g_modify_flag = (res_x == 1024) ? 1 : 0;

    return SUCCESS;
}

void touch_scan(void)
{
    touch_data_t tmp;
    uint8_t buf[2 + 8 * TOUCH_MAX_POINTS];

    gt9xx_read_reg(TOUCH_GT9XX_READ_ADDR, 2 + 8 * TOUCH_MAX_POINTS, buf);
    gt9xx_write_data(TOUCH_GT9XX_READ_ADDR, 0);

    tmp.num = buf[0] & 0x0F;

    if (tmp.num >= 1 && tmp.num <= 5) {
        for (uint8_t i = 0; i < tmp.num; i++) {
            tmp.y[i] = (buf[5 + 8 * i] << 8) | buf[4 + 8 * i];
            tmp.x[i] = (buf[3 + 8 * i] << 8) | buf[2 + 8 * i];

            if (g_modify_flag) {
                tmp.y[i] = (uint16_t)(tmp.y[i] * 0.80f);
                tmp.x[i] = (uint16_t)(tmp.x[i] * 0.78f);
            }
        }
        tmp.flag = 1;
    } else {
        tmp.flag = 0;
    }

    TOUCH_LOCK();
    g_touch_data = tmp;
    TOUCH_UNLOCK();
}

bool touch_is_touch(void)
{
    bool ret;
    TOUCH_LOCK();
    ret = (g_touch_data.flag != 0);
    TOUCH_UNLOCK();
    return ret;
}

uint8_t touch_get_num(void)
{
    uint8_t num;
    TOUCH_LOCK();
    num = g_touch_data.num;
    TOUCH_UNLOCK();
    return num;
}

void touch_read_xy(uint8_t idx, uint16_t *x, uint16_t *y)
{
    if (idx >= TOUCH_MAX_POINTS) return;

    TOUCH_LOCK();
    *x = g_touch_data.x[idx];
    *y = g_touch_data.y[idx];
    TOUCH_UNLOCK();
}
