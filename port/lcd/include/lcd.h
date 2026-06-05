#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "main.h"

/* 显示分辨率 (匹配 CubeMX LTDC 配置) */
#define LCD_WIDTH   800
#define LCD_HEIGHT  480

/* RGB565 颜色 */
#define LCD_WHITE       0xFFFF
#define LCD_BLACK       0x0000
#define LCD_BLUE        0x001F
#define LCD_GREEN       0x07E0
#define LCD_RED         0xF800
#define LCD_CYAN        0x07FF
#define LCD_MAGENTA     0xF81F
#define LCD_YELLOW      0xFFE0
#define LCD_GREY        0x8410
#define LIGHT_BLUE      0x8EDC
#define LIGHT_GREEN     0x97F0
#define LIGHT_RED       0xFC10
#define LIGHT_CYAN      0x8FFC
#define LIGHT_MAGENTA   0xFCDC
#define LIGHT_YELLOW    0xFFF0
#define LIGHT_GREY      0xB596
#define DARK_BLUE       0x0010
#define DARK_GREEN      0x0400
#define DARK_RED        0x8000
#define DARK_CYAN       0x0410
#define DARK_MAGENTA    0x8010
#define DARK_YELLOW     0x8400
#define DARK_GREY       0x4208

/* 背光控制 (使用 main.h 中 CubeMX 生成的引脚宏) */
#define LCD_BL_ON()   HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, GPIO_PIN_SET)
#define LCD_BL_OFF()  HAL_GPIO_WritePin(LCD_BK_GPIO_Port, LCD_BK_Pin, GPIO_PIN_RESET)

/* 字体结构体 */
typedef struct {
    const uint8_t *table;
    uint16_t width;
    uint16_t height;
    uint16_t bytes_per_char;
} lcd_font_t;

/* 字体声明 */
extern lcd_font_t LCD_Font_12x6;
extern lcd_font_t LCD_Font_16x8;

/* 初始化 (含 LTDC 时钟、引脚、图层、背光) */
void lcd_init(void);

/* 颜色设置 */
void lcd_set_color(uint16_t color);
void lcd_set_back_color(uint16_t color);

/* 区域刷新 (DMA2D RGB565 → RGB565, 用于 LVGL) */
void lcd_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

/* 清屏 (DMA2D) */
void lcd_clear(void);
void lcd_clear_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* 像素 */
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
uint16_t lcd_read_point(uint16_t x, uint16_t y);

/* 2D 绘图 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void lcd_draw_circle(uint16_t x, uint16_t y, uint16_t r);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r);

/* 文字 */
void lcd_set_font(lcd_font_t *font);
void lcd_show_char(uint16_t x, uint16_t y, char ch);
void lcd_show_string(uint16_t x, uint16_t y, const char *str);
void lcd_show_num(uint16_t x, uint16_t y, int32_t num, uint8_t len);

void lcd_test(void);

#endif
