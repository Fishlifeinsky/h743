/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 (ST LTDC 直通模式 + 触摸输入)
  *
  *          显示: lv_st_ltdc_create_direct() 双显存 + vsync 交换,
  *          输入: GT911 触摸屏注册为 LV_INDEV_TYPE_POINTER.
  *
  *          心跳: BSP_FREERTOS_ENABLED=1 → FreeRTOS tick hook → lv_tick_inc()
  *                BSP_FREERTOS_ENABLED=0 → lv_tick_set_cb(lv_port_tick_get)
  */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port.h"
#include "lvgl.h"
#include <string.h>
#include "mem.h"
#include "stm32h7xx_hal.h"
#include "src/drivers/display/st_ltdc/lv_st_ltdc.h"
#include "ltdc.h"
#include "touch.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES  800
#define MY_DISP_VER_RES  480
#define FB_SIZE          (MY_DISP_HOR_RES * MY_DISP_VER_RES * 2)

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool g_lv_initialized = false;

/* 第二帧缓冲 (第一帧缓冲复用 lcd.c 的 LCD_MEM_ADDRESS) */
MEM_SDRAM_DATA_SECTION
static uint8_t fb_buf2[FB_SIZE];

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

//--------------------------------------------------------------------+
// Tick 心跳
//--------------------------------------------------------------------+

#if BSP_FREERTOS_ENABLED

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

#else

uint32_t lv_port_tick_get(void)
{
    return HAL_GetTick();
}

#endif

//--------------------------------------------------------------------+
// LVGL 初始化
//--------------------------------------------------------------------+

void lv_port_init(void)
{
    if (g_lv_initialized) return;

    lv_init();

#if !BSP_FREERTOS_ENABLED
    lv_tick_set_cb(lv_port_tick_get);
#endif

    lv_port_disp_init();
    lv_port_indev_init();

    /* 默认主题: 白底黑字 */
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);

    g_lv_initialized = true;
    DEBUG_PRINT("lv_port: init ok, ST LTDC direct 800x480 + touch\r\n");
}

//--------------------------------------------------------------------+
// 显示初始化
//--------------------------------------------------------------------+

void lv_port_disp_init(void)
{
    memset(fb_buf2, 0, FB_SIZE);

    extern uint8_t LCD_MEM_ADDRESS[];
    lv_st_ltdc_create_direct((void *)LCD_MEM_ADDRESS, fb_buf2, 0);
}

//--------------------------------------------------------------------+
// 触摸输入初始化
//--------------------------------------------------------------------+

void lv_port_indev_init(void)
{
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    if (touch_is_touch()) {
        uint16_t tx, ty;
        touch_read_xy(0, &tx, &ty);
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = tx;
        data->point.y = ty;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
