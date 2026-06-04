/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 (基于 lv_port_disp_template.c)
  *
  *          Partial render 模式：LVGL 渲染到 DTCM 局部缓存，
  *          flush 回调拷贝到 SDRAM 显存，LTDC 持续扫描输出。
  *
  *          心跳源: BSP_FREERTOS_ENABLED=1 → FreeRTOS tick hook → lv_tick_inc()
  *                  BSP_FREERTOS_ENABLED=0 → HAL_GetTick() → lv_port_tick_get()
  */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port.h"
#include "lvgl.h"
#include <string.h>
#include "BSP/config.h"
#include "mem.h"
#include "stm32h7xx_hal.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES  800
#define MY_DISP_VER_RES  480
#define BUF_ROWS         40    /* DTCM 缓存行数 (800*40*4 = 128KB) */

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool g_lv_initialized = false;

/* DTCM 缓存: 128KB, 零等待, Cortex-M7 专用 */
MEM_SRAM_DTCM_SECTION
static uint8_t disp_buf_1[MY_DISP_HOR_RES * BUF_ROWS * 4];

/* 双缓冲需要 256KB > DTCM 128KB, 仅用单缓冲 */

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

    /* 创建显示 — partial render: DTCM 缓存 → flush 拷贝到 SDRAM 显存 */
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_flush_cb(disp, disp_flush);

    lv_display_set_buffers(disp,
                           disp_buf_1, NULL,
                           sizeof(disp_buf_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* 默认主题: 白底黑字 */
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);

    g_lv_initialized = true;
    DEBUG_PRINT("LVGL: port init ok, partial render 800x480 ARGB8888, DTCM buf %d rows\r\n",
                BUF_ROWS);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* 将 px_map 中的渲染结果逐行拷贝到 SDRAM 显存 */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    extern uint8_t LCD_MEM_ADDRESS[];
    uint32_t fb_w = MY_DISP_HOR_RES;
    uint32_t w    = lv_area_get_width(area);
    uint32_t h    = lv_area_get_height(area);
    uint32_t line = w * 4; /* ARGB8888 */

    for (uint32_t y = 0; y < h; y++) {
        uint32_t * dst = (uint32_t *)&LCD_MEM_ADDRESS[((area->y1 + y) * fb_w + area->x1) * 4];
        uint32_t * src = (uint32_t *)(px_map + y * line);
        for (uint32_t x = 0; x < w; x++) {
            dst[x] = src[x];
        }
        SCB_CleanInvalidateDCache_by_Addr(dst, (int32_t)(w * 4));
    }

    lv_display_flush_ready(disp);
}
