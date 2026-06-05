/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 — 使用 LVGL 内置 ST LTDC 驱动
  *
  *          lv_st_ltdc_create_direct() 直通模式 + 双显存,
  *          驱动内部处理 vsync 影子寄存器交换和同步,
  *          SDRAM 透写无需手动 Cache 维护.
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

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES  800
#define MY_DISP_VER_RES  480
#define FB_SIZE          (MY_DISP_HOR_RES * MY_DISP_VER_RES * 4)

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

    /* 后缓冲初始化 */
    memset(fb_buf2, 0, FB_SIZE);

    /* 使用 LVGL 内置 ST LTDC 驱动 — DIRECT 直通模式 + 双显存
     * 驱动自动处理: vsync 影子寄存器交换 / LTDC reload 中断同步 */
    {
        extern uint8_t LCD_MEM_ADDRESS[];
        lv_st_ltdc_create_direct((void *)LCD_MEM_ADDRESS, fb_buf2, 0);
    }

    /* 默认主题: 白底黑字 */
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);

    g_lv_initialized = true;
    DEBUG_PRINT("LVGL: port init ok, ST LTDC direct dual buf 800x480\r\n");
}
