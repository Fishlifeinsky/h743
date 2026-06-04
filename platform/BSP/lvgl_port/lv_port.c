/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 (基于 lv_port_disp_template.c)
  *
  *          Direct render 模式：LVGL 直接渲染到 SDRAM 显存，
  *          LTDC 持续扫描输出，flush 无需拷贝像素。
  *
  *          心跳源: BSP_FREERTOS_ENABLED=1 → FreeRTOS tick hook → lv_tick_inc()
  *                  BSP_FREERTOS_ENABLED=0 → HAL_GetTick() → lv_port_tick_get()
  */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port.h"
#include "lvgl.h"
#include "BSP/config.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES  800
#define MY_DISP_VER_RES  480

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool g_lv_initialized = false;

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

    /* 创建显示 — direct render 到 SDRAM 显存 (LCD_MEM_ADDRESS, 已在 lcd_init 中配置) */
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    extern uint8_t LCD_MEM_ADDRESS[];
    uint32_t buf_size = MY_DISP_HOR_RES * MY_DISP_VER_RES * 4; /* ARGB8888 */
    lv_display_set_buffers(disp, LCD_MEM_ADDRESS, NULL, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);

    /* 默认主题: 白底黑字 */
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);

    g_lv_initialized = true;
    DEBUG_PRINT("LVGL: port init ok, direct render 800x480 ARGB8888\r\n");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* direct render 模式: LVGL 直接写入显存, LTDC 持续扫描, flush 无需拷贝 */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void)area;
    (void)px_map;
    lv_display_flush_ready(disp);
}
