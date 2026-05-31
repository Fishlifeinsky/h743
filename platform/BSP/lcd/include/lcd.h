#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "main.h"

/* 显示分辨率 (匹配 CubeMX LTDC 配置) */
#define LCD_WIDTH   800
#define LCD_HEIGHT  480

/* ARGB8888 颜色 (0xAARRGGBB) */
#define LCD_WHITE       0xFFFFFFFF
#define LCD_BLACK       0xFF000000
#define LCD_BLUE        0xFF0000FF
#define LCD_GREEN       0xFF00FF00
#define LCD_RED         0xFFFF0000
#define LCD_CYAN        0xFF00FFFF
#define LCD_MAGENTA     0xFFFF00FF
#define LCD_YELLOW      0xFFFFFF00
#define LCD_GREY        0xFF2C2C2C
#define LIGHT_BLUE      0xFF8080FF
#define LIGHT_GREEN     0xFF80FF80
#define LIGHT_RED       0xFFFF8080
#define LIGHT_CYAN      0xFF80FFFF
#define LIGHT_MAGENTA   0xFFFF80FF
#define LIGHT_YELLOW    0xFFFFFF80
#define LIGHT_GREY      0xFFA3A3A3
#define DARK_BLUE       0xFF000080
#define DARK_GREEN      0xFF008000
#define DARK_RED        0xFF800000
#define DARK_CYAN       0xFF008080
#define DARK_MAGENTA    0xFF800080
#define DARK_YELLOW     0xFF808000
#define DARK_GREY       0xFF404040

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
void lcd_set_color(uint32_t color);
void lcd_set_back_color(uint32_t color);

/* 区域刷新 (DMA2D RGB565 → ARGB8888, 用于 LVGL) */
void lcd_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

/* 清屏 (DMA2D) */
void lcd_clear(void);
void lcd_clear_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* 像素 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t lcd_read_point(uint16_t x, uint16_t y);

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
