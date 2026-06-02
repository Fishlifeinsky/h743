/**
  * @file    lcd.c
  * @brief   LCD 显示驱动模块 (含 LTDC 控制器初始化)
  *
  *          显存位于外部 SDRAM (0xC0000000), ARGB8888 单层, 800x480@60fps。
  *          DMA2D 硬件加速填充/清屏, 逐像素点阵 ASCII 字体渲染。
  *
  *          时序适配反客 FK743M2-IIT6 核心板 (HSE 25MHz → LTDC 33MHz)。
  */

#include "lcd.h"
#include "lcd_fonts.h"
#include "ltdc.h"
#include "mem.h"
#include <string.h>
#include <stdio.h>

/* LTDC 时序参数 (适配 800x480 RGB 屏, 参考反客示例) */
#define LCD_HSW  1
#define LCD_VSW  1
#define LCD_HBP  80
#define LCD_VBP  40
#define LCD_HFP  200
#define LCD_VFP  22

/* 背光引脚 — PH6, 推挽输出 */
#define LCD_BK_PIN       GPIO_PIN_6
#define LCD_BK_PORT      GPIOH
#define LCD_BK_CLK_EN()  __HAL_RCC_GPIOH_CLK_ENABLE()

//--------------------------------------------------------------------+
// 全局句柄与显存
//--------------------------------------------------------------------+

uint8_t LCD_MEM_ADDRESS[800 * 480 * 4] MEM_SDRAM_DATA_SECTION;

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static uint32_t g_color;
static uint32_t g_back_color;
static lcd_font_t *g_font = &LCD_Font_16x8;

/* 显存基址 */
#define FB_BASE  ((uint32_t)LCD_MEM_ADDRESS)

//--------------------------------------------------------------------+
// DMA2D 操作
//--------------------------------------------------------------------+

static void DMA2D_Wait(void)
{
    while (DMA2D->CR & DMA2D_CR_START);
}

static void DMA2D_Fill(void *dst, uint32_t color, uint16_t w, uint16_t h, uint16_t off)
{
    DMA2D->CR     &= ~(DMA2D_CR_START);
    DMA2D->CR      = DMA2D_R2M;
    DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;
    DMA2D->OOR     = off;
    DMA2D->OMAR    = (uint32_t)dst;
    DMA2D->NLR     = ((uint32_t)w << 16) | (uint32_t)h;
    DMA2D->OCOLR   = color;
    DMA2D->CR     |= DMA2D_CR_START;
    DMA2D_Wait();
}

/**
  * @brief DMA2D 内存到内存拷贝 (含像素格式转换)
  * @note  RGB565 → ARGB8888, 用于 LVGL flush 回调
  */
static void DMA2D_Blit(void *dst, const void *src, uint16_t w, uint16_t h,
                        uint16_t dst_off, uint16_t src_off)
{
    DMA2D->CR      = DMA2D_M2M_PFC;
    DMA2D->FGMAR   = (uint32_t)src;
    DMA2D->OMAR    = (uint32_t)dst;
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
    DMA2D->OPFCCR  = LTDC_PIXEL_FORMAT_ARGB8888;
    DMA2D->FGOR    = src_off;
    DMA2D->OOR     = dst_off;
    DMA2D->NLR     = ((uint32_t)w << 16) | (uint32_t)h;
    DMA2D->CR     |= DMA2D_CR_START;
    DMA2D_Wait();
}

//--------------------------------------------------------------------+
// 初始化
//--------------------------------------------------------------------+

/**
  * @brief LCD 模块初始化
  * @note  配置 PLL3 → LTDC 33MHz, 初始化 LTDC 外设/DMA2D/背光
  */
void lcd_init(void)
{
    LTDC_LayerCfgTypeDef pLayerCfg = {0};

    /* 使能 DMA2D 时钟 */
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /* 背光引脚初始化 */
    LCD_BK_CLK_EN();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = LCD_BK_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_BK_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LCD_BK_PORT, LCD_BK_PIN, GPIO_PIN_RESET);

    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc.Init.HorizontalSync = 1 - 1;
    hltdc.Init.VerticalSync = 1 - 1;
    hltdc.Init.AccumulatedHBP = 80 + 1 - 1;
    hltdc.Init.AccumulatedVBP = 40 + 1 - 1;
    hltdc.Init.AccumulatedActiveW = 800 + 1 + 80 - 1;
    hltdc.Init.AccumulatedActiveH = 480 + 1 + 40 - 1;
    hltdc.Init.TotalWidth = 800 + 1 + 80 + 200 - 1;
    hltdc.Init.TotalHeigh = 480 + 1 + 40 + 22 - 1;
    hltdc.Init.Backcolor.Blue = 0;
    hltdc.Init.Backcolor.Green = 0;
    hltdc.Init.Backcolor.Red = 0;
    if (HAL_LTDC_Init(&hltdc) != HAL_OK)
    {
        Error_Handler();
    }
    pLayerCfg.WindowX0 = 0;
    pLayerCfg.WindowX1 = 800;
    pLayerCfg.WindowY0 = 0;
    pLayerCfg.WindowY1 = 480;
    pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
    pLayerCfg.Alpha = 255;
    pLayerCfg.Alpha0 = 0;
    pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    pLayerCfg.FBStartAdress = (uint32_t)LCD_MEM_ADDRESS;
    pLayerCfg.ImageWidth = 800;
    pLayerCfg.ImageHeight = 480;
    pLayerCfg.Backcolor.Blue = 0;
    pLayerCfg.Backcolor.Green = 0;
    pLayerCfg.Backcolor.Red = 0;
    if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
    {
        Error_Handler();
    }
    /* ARGB8888 需要抖动优化 */
    HAL_LTDC_EnableDither(&hltdc);

    /* 初始化绘图上下文 */
    g_color      = LCD_WHITE;
    g_back_color = LCD_BLACK;
    g_font       = &LCD_Font_16x8;

    lcd_clear();
    LCD_BL_ON();

    /* USER CODE BEGIN LCD_Init 1 */

    /* USER CODE END LCD_Init 1 */
}

//--------------------------------------------------------------------+
// 颜色设置
//--------------------------------------------------------------------+

void lcd_set_color(uint32_t color)
{
    g_color = color;
}

void lcd_set_back_color(uint32_t color)
{
    g_back_color = color;
}

//--------------------------------------------------------------------+
// 清屏 (DMA2D)
//--------------------------------------------------------------------+

void lcd_clear(void)
{
    while (LTDC->CDSR != 0x00000001);
    DMA2D_Fill((void *)FB_BASE, g_back_color, LCD_WIDTH, LCD_HEIGHT, 0);
}

void lcd_clear_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint32_t addr = FB_BASE + 4 * (LCD_WIDTH * y + x);
    DMA2D_Fill((void *)addr, g_back_color, w, h, LCD_WIDTH - w);
}

//--------------------------------------------------------------------+
// 像素
//--------------------------------------------------------------------+

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    *(__IO uint32_t *)(FB_BASE + 4 * (x + y * LCD_WIDTH)) = color;
}

uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    return *(__IO uint32_t *)(FB_BASE + 4 * (x + y * LCD_WIDTH));
}

//--------------------------------------------------------------------+
// 区域刷新 (DMA2D 像素格式转换)
//--------------------------------------------------------------------+

/**
  * @brief 将 RGB565 像素数组写入显存指定区域
  * @note  DMA2D 硬件转换 RGB565 → ARGB8888, 用于 LVGL flush 回调
  * @param x,y  目标区域左上角坐标
  * @param w,h  区域宽高 (像素)
  * @param data RGB565 像素数组, 每像素 2 字节, 行连续排列
  */
void lcd_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
    uint32_t addr = FB_BASE + 4 * (LCD_WIDTH * y + x);
    DMA2D_Blit((void *)addr, data, w, h, LCD_WIDTH - w, 0);
}

//--------------------------------------------------------------------+
// 直线 (Bresenham)
//--------------------------------------------------------------------+

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int16_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int16_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        lcd_draw_point(x1, y1, g_color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

//--------------------------------------------------------------------+
// 矩形
//--------------------------------------------------------------------+

void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    lcd_draw_line(x, y, x + w, y);
    lcd_draw_line(x, y + h, x + w, y + h);
    lcd_draw_line(x, y, x, y + h);
    lcd_draw_line(x + w, y, x + w, y + h);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint32_t addr = FB_BASE + 4 * (LCD_WIDTH * y + x);
    DMA2D_Fill((void *)addr, g_color, w, h, LCD_WIDTH - w);
}

//--------------------------------------------------------------------+
// 圆形
//--------------------------------------------------------------------+

void lcd_draw_circle(uint16_t x, uint16_t y, uint16_t r)
{
    int Xadd = -r, Yadd = 0, err = 2 - 2 * r;
    do {
        lcd_draw_point(x - Xadd, y + Yadd, g_color);
        lcd_draw_point(x + Xadd, y + Yadd, g_color);
        lcd_draw_point(x + Xadd, y - Yadd, g_color);
        lcd_draw_point(x - Xadd, y - Yadd, g_color);

        int e2 = err;
        if (e2 <= Yadd) { err += ++Yadd * 2 + 1; if (-Xadd == Yadd && e2 <= Xadd) e2 = 0; }
        if (e2 > Xadd)  { err += ++Xadd * 2 + 1; }
    } while (Xadd <= 0);
}

void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r)
{
    int32_t D  = 3 - (r << 1);
    uint32_t CurX = 0, CurY = r;

    while (CurX <= CurY) {
        if (CurY > 0) {
            lcd_draw_line(x - CurX, y - CurY, x - CurX, y - CurY + 2 * CurY);
            lcd_draw_line(x + CurX, y - CurY, x + CurX, y - CurY + 2 * CurY);
        }
        if (CurX > 0) {
            lcd_draw_line(x - CurY, y - CurX, x - CurY, y - CurX + 2 * CurX);
            lcd_draw_line(x + CurY, y - CurX, x + CurY, y - CurX + 2 * CurX);
        }
        if (D < 0) { D += (CurX << 2) + 6; }
        else       { D += ((CurX - CurY) << 2) + 10; CurY--; }
        CurX++;
    }
    lcd_draw_circle(x, y, r);
}

//--------------------------------------------------------------------+
// 文字
//--------------------------------------------------------------------+

void lcd_set_font(lcd_font_t *font)
{
    if (font) g_font = font;
}

void lcd_show_char(uint16_t x, uint16_t y, char ch)
{
    uint16_t Xaddr = x;
    const uint8_t *p = &g_font->table[(ch - 32) * g_font->bytes_per_char];

    for (uint16_t i = 0; i < g_font->bytes_per_char; i++) {
        uint8_t byte = *p++;
        for (int b = 0; b < 8; b++) {
            lcd_draw_point(Xaddr, y, (byte & 0x01) ? g_color : g_back_color);
            byte >>= 1;
            Xaddr++;
            if (Xaddr - x == g_font->width) {
                Xaddr = x;
                y++;
                break;
            }
        }
    }
}

void lcd_show_string(uint16_t x, uint16_t y, const char *str)
{
    while (*str) {
        lcd_show_char(x, y, *str);
        x += g_font->width;
        str++;
    }
}

void lcd_show_num(uint16_t x, uint16_t y, int32_t num, uint8_t len)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%*d", len, num);
    lcd_show_string(x, y, buf);
}
