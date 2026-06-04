/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 — partial render + DMA2D flush
  *
  *          LVGL 渲染到 DTCM 局部缓存 (零等待),
  *          flush 回调通过 DMA2D 拷贝到 SDRAM 显存,
  *          DMA2D 绕过 CPU D-Cache 直接写 SDRAM,
  *          LTDC 常量扫描输出到 LCD 面板.
  *
  *          心跳: BSP_FREERTOS_ENABLED=1 → FreeRTOS tick hook → lv_tick_inc()
  *                BSP_FREERTOS_ENABLED=0 → HAL_GetTick() → lv_port_tick_get()
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
#define BUF_ROWS         40

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool g_lv_initialized = false;

/* DTCM 渲染缓存: 128KB, 零等待, Cortex-M7 AHBS 口可被 DMA2D 访问 */
MEM_SRAM_DTCM_SECTION
static uint8_t disp_buf_1[MY_DISP_HOR_RES * BUF_ROWS * 4];

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

    /* 创建显示 — partial render: DTCM 缓存 → DMA2D → SDRAM 显存 */
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
    DEBUG_PRINT("LVGL: port init ok, DMA2D flush 800x480 ARGB8888, DTCM buf %d rows\r\n",
                BUF_ROWS);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* DMA2D 拷贝 ARGB8888 矩形: px_map (DTCM) → SDRAM 帧缓冲 (绕过 D-Cache) */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    extern uint8_t LCD_MEM_ADDRESS[];
    uint32_t fb_w = MY_DISP_HOR_RES;
    uint32_t w    = lv_area_get_width(area);
    uint32_t h    = lv_area_get_height(area);

    uint32_t dst_addr = (uint32_t)&LCD_MEM_ADDRESS[(area->y1 * fb_w + area->x1) * 4];

    DMA2D->CR     &= ~(DMA2D_CR_START);
    DMA2D->CR      = DMA2D_M2M_PFC;
    DMA2D->FGMAR   = (uint32_t)px_map;
    DMA2D->OMAR    = dst_addr;
    DMA2D->FGPFCCR = 0;    /* ARGB8888 */
    DMA2D->OPFCCR  = 0;    /* ARGB8888 */
    DMA2D->FGOR    = 0;    /* px_map 行连续, 无行间偏移 */
    DMA2D->OOR     = fb_w - w; /* 目标帧缓冲行间跳过 (fb_w - w) 像素 */
    DMA2D->NLR     = ((uint32_t)w << 16) | (uint32_t)h;
    DMA2D->CR     |= DMA2D_CR_START;
    while (DMA2D->CR & DMA2D_CR_START);

    lv_display_flush_ready(disp);
}
