/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 — DIRECT 直通模式 + 双显存 + vsync 交换
  *
  *          LVGL 直接渲染到 SDRAM 帧缓冲 (LV_DISPLAY_RENDER_MODE_DIRECT),
  *          两块 1.5MB 显存交替使用, LTDC vsync 影子寄存器交换前后缓冲.
  *          SDRAM MPU 配置为透写 (Write-Through), 无需手动 Cache 维护.
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
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

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

    /* DIRECT 模式 + 双显存: LVGL 直接渲染到 SDRAM 帧缓冲 */
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_flush_cb(disp, disp_flush);

    {
        extern uint8_t LCD_MEM_ADDRESS[];
        lv_display_set_buffers(disp,
                               (void *)LCD_MEM_ADDRESS, fb_buf2,
                               FB_SIZE,
                               LV_DISPLAY_RENDER_MODE_DIRECT);

        /* 后缓冲初始化为黑色, 与 lcd_clear 保持一致 */
        memset(fb_buf2, 0, FB_SIZE);
    }

    /* 默认主题: 白底黑字 */
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);

    g_lv_initialized = true;
    DEBUG_PRINT("LVGL: port init ok, DIRECT dual buf 800x480 ARGB8888 WT\r\n");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* DIRECT 模式 flush: 最后一帧时交换前后缓冲 (LTDC vsync 影子寄存器) */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* 透写模式下 CPU 写 SDRAM 同步更新物理内存, 无需 Cache 维护 */

    if (lv_display_flush_is_last(disp)) {
        /* px_map 在某帧缓冲内, 判断属于 buf1(LCD_MEM_ADDRESS) 还是 buf2(fb_buf2) */
        extern uint8_t LCD_MEM_ADDRESS[];
        void *rendered = (px_map >= fb_buf2 && px_map < fb_buf2 + FB_SIZE)
                         ? (void *)fb_buf2 : (void *)LCD_MEM_ADDRESS;

        extern LTDC_HandleTypeDef hltdc;
        HAL_LTDC_SetAddress(&hltdc, (uint32_t)rendered, 0);
        __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
    }

    lv_display_flush_ready(disp);
}
