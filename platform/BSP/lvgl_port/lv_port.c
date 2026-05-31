/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 (STM32H7 + LTDC + DMA2D + FreeRTOS)
  *
  *          Direct render 模式：LVGL 直接渲染到 LTDC 层显存 (SDRAM)。
  *          DMA2D 硬件加速绘制 fill/blit，无需额外 flush。
  */

#include "lv_port.h"
#include "lvgl.h"
#include "ltdc.h"
#include "lcd.h"

#include "BSP/config.h"

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

DMA2D_HandleTypeDef hdma2d = {0};       // ST LVGL 驱动引用 (DMA2D flush 模式)
static bool g_lv_initialized = false;

//--------------------------------------------------------------------+
// 内部函数声明
//--------------------------------------------------------------------+
static void lv_port_display_init(void);

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

uint32_t lv_port_tick_get(void)
{
    return HAL_GetTick();
}

void lv_port_init(void)
{
    if (g_lv_initialized) return;

    lv_init();
    lv_port_display_init();
    g_lv_initialized = true;

    DEBUG_PRINT("LVGL: port init ok, LTDC+DMA2D, ARGB8888\r\n");
}

//--------------------------------------------------------------------+
// 显示初始化 (ST LTDC 驱动, Direct Render 模式)
//--------------------------------------------------------------------+

static void lv_port_display_init(void)
{
    // direct 模式: LVGL 直接渲染到层显存, HLDC Layer0 已在 lcd_init() 中配置
    extern uint8_t LCD_MEM_ADDRESS[];
    lv_st_ltdc_create_direct((void *)LCD_MEM_ADDRESS, NULL, 0);

    // 设置默认主题
    lv_theme_default_init(NULL,
                          lv_color_make(0xFF, 0xFF, 0xFF),
                          lv_color_make(0x00, 0x00, 0x00),
                          0,
                          &lv_font_montserrat_14);
}
