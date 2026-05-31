/**
  * @file    lv_conf.h
  * @brief   LVGL v9 配置文件 (STM32H7 + LTDC + DMA2D + ARGB8888)
  *
  *          配置项按 LVGL 模块分组，仅列出与默认值不同的项。
  *          完整配置项列表见 3rd/lvgl/lv_conf_template.h
  */

#ifndef LV_CONF_H
#define LV_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 显示色深 — ARGB8888
//--------------------------------------------------------------------+

#define LV_COLOR_DEPTH 32
#define LV_COLOR_TRANSP LV_COLOR_MAKE(0x00, 0x00, 0x00)
#define LV_COLOR_BLACK  LV_COLOR_MAKE(0x00, 0x00, 0x00)
#define LV_COLOR_WHITE  LV_COLOR_MAKE(0xFF, 0xFF, 0xFF)
#define LV_COLOR_DEFAULT_OPAQUE 0xFF

//--------------------------------------------------------------------+
// OS 抽象层 — FreeRTOS
//--------------------------------------------------------------------+

#define LV_USE_OS   LV_OS_FREERTOS

//--------------------------------------------------------------------+
// 内存
//--------------------------------------------------------------------+

#define LV_MEM_SIZE (256 * 1024)        // 256KB 内部 SRAM

//--------------------------------------------------------------------+
// Tick — 自定义 (HAL_GetTick)
//--------------------------------------------------------------------+

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <stdint.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (lv_port_tick_get())

//--------------------------------------------------------------------+
// 日志 — 关闭
//--------------------------------------------------------------------+

#define LV_USE_LOG 0

//--------------------------------------------------------------------+
// STM32 LTDC 显示驱动
//--------------------------------------------------------------------+

#define LV_USE_ST_LTDC 1
#define LV_ST_LTDC_USE_DMA2D_FLUSH 0   // direct render 模式不需要 DMA2D flush

//--------------------------------------------------------------------+
// DMA2D 绘制加速
//--------------------------------------------------------------------+

#define LV_USE_DRAW_DMA2D 1

//--------------------------------------------------------------------+
// 字体 — 仅保留内置 Montserrat 14
//--------------------------------------------------------------------+

#define LV_USE_FONT_MONTSERRAT_8   0
#define LV_USE_FONT_MONTSERRAT_10  0
#define LV_USE_FONT_MONTSERRAT_12  0
#define LV_USE_FONT_MONTSERRAT_16  0
#define LV_USE_FONT_MONTSERRAT_18  0
#define LV_USE_FONT_MONTSERRAT_20  0
#define LV_USE_FONT_MONTSERRAT_22  0
#define LV_USE_FONT_MONTSERRAT_24  0
#define LV_USE_FONT_MONTSERRAT_26  0
#define LV_USE_FONT_MONTSERRAT_28  0
#define LV_USE_FONT_MONTSERRAT_30  0
#define LV_USE_FONT_MONTSERRAT_32  0
#define LV_USE_FONT_MONTSERRAT_34  0
#define LV_USE_FONT_MONTSERRAT_36  0
#define LV_USE_FONT_MONTSERRAT_38  0
#define LV_USE_FONT_MONTSERRAT_40  0
#define LV_USE_FONT_MONTSERRAT_42  0
#define LV_USE_FONT_MONTSERRAT_44  0
#define LV_USE_FONT_MONTSERRAT_46  0
#define LV_USE_FONT_MONTSERRAT_48  0

//--------------------------------------------------------------------+
// 文件系统 — 关闭 (通过 VFS 间接使用)
//--------------------------------------------------------------------+

#define LV_USE_FS_FATFS 0
#define LV_USE_FS_STDIO 0

//--------------------------------------------------------------------+
// Widget — 仅释放常用控件
//--------------------------------------------------------------------+

#define LV_USE_BUTTON     1
#define LV_USE_LABEL      1
#define LV_USE_SWITCH     1
#define LV_USE_SLIDER     1

#define LV_USE_ANIMIMG    0
#define LV_USE_ARC        0
#define LV_USE_BAR        0
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR   0
#define LV_USE_CANVAS     0
#define LV_USE_CHART      0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   0
#define LV_USE_IMAGE      0
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD   0
#define LV_USE_LED        0
#define LV_USE_LINE       0
#define LV_USE_LIST       0
#define LV_USE_LOTTIE     0
#define LV_USE_MENU       0
#define LV_USE_MSGBOX     0
#define LV_USE_ROLLER     0
#define LV_USE_SCALE      0
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    0
#define LV_USE_TEXTAREA   0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

#ifdef __cplusplus
}
#endif

#endif
