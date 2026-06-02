/**
  * @file    lv_port.c
  * @brief   LVGL v9 移植 (STM32H7 + LTDC + DMA2D + FreeRTOS)
  *
  *          Direct render 模式：LVGL 直接渲染到 LTDC 层显存 (SDRAM)。
  *          DMA2D 硬件加速绘制 fill/blit，无需额外 flush。
  *
  *          心跳源: BSP_FREERTOS_ENABLED=1 → FreeRTOS tick hook → lv_tick_inc()
  *                  BSP_FREERTOS_ENABLED=0 → HAL_GetTick() → lv_port_tick_get()
  */

#include "lv_port.h"
#include "lvgl.h"
#include "ltdc.h"
#include "lcd.h"
#include "stm32h7xx_hal_dma.h"

#include "BSP/config.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static bool g_lv_initialized = false;

//--------------------------------------------------------------------+
// 内部函数声明
//--------------------------------------------------------------------+
static void lv_port_display_init(void);

//--------------------------------------------------------------------+
// Tick 心跳
//--------------------------------------------------------------------+

#if BSP_FREERTOS_ENABLED

// FreeRTOS 模式下由 tick hook 调用 lv_tick_inc(1ms)
void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

#else

// 裸机模式下使用 HAL tick
uint32_t lv_port_tick_get(void)
{
    return HAL_GetTick();
}

#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

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
