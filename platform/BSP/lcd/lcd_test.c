#include "lcd.h"

#include "system_delay.h"
#include "BSP/config.h"

#if USE_BSP_UNIT_TESTING

static void delay_ms(uint32_t ms)
{
    system_delay_ms(ms);
}

//--------------------------------------------------------------------+
// 清屏色测试
//--------------------------------------------------------------------+

static void lcd_test_clear(void)
{
    DEBUG_PRINT("\r\n[LCD] 清屏色测试\r\n");

    uint32_t colors[] = {
        LCD_RED, LCD_GREEN, LCD_BLUE, LCD_CYAN, LCD_MAGENTA, LCD_YELLOW,
        LCD_WHITE, LCD_BLACK,
    };
    const char *names[] = {
        "RED", "GREEN", "BLUE", "CYAN", "MAGENTA", "YELLOW",
        "WHITE", "BLACK",
    };

    for (int i = 0; i < 8; i++) {
        lcd_set_color(colors[i]);
        lcd_fill_rect(0, 0, 800, 480);
        lcd_set_color(i < 4 ? LCD_BLACK : LCD_WHITE);
        lcd_show_string(10, 10, names[i]);
        delay_ms(500);
    }

    /* 整帧刷新时间测试 (DWT 周期计数) */
    {
        DEBUG_PRINT("\r\n[LCD] 整帧刷新时间测试 (DWT, HCLK=240MHz)\r\n");

        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        uint32_t t0, t1, c;

        /* DMA2D 填充整屏 */
        lcd_set_color(LCD_RED);
        t0 = DWT->CYCCNT;
        lcd_fill_rect(0, 0, 800, 480);
        t1 = DWT->CYCCNT;
        c = t1 - t0;
        DEBUG_PRINT("  DMA2D fill 800x480: %lu us (%lu cyc)\r\n",
                    c / 240, c);

        /* 逐点填充全屏 800x480 */
        lcd_set_color(LCD_GREEN);
        t0 = DWT->CYCCNT;
        for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
            for (uint16_t x = 0; x < LCD_WIDTH; x++) {
                lcd_draw_point(x, y, (uint32_t)((x ^ y) | 0xFF000000));
            }
        }
        t1 = DWT->CYCCNT;
        c = t1 - t0;
        DEBUG_PRINT("  逐点填充 800x480: %lu us (%lu cyc)\r\n",
                    c / 240, c);

        delay_ms(2000);
    }

    lcd_set_color(LCD_WHITE);
    lcd_fill_rect(0, 0, 800, 480);
}

//--------------------------------------------------------------------+
// 文字测试
//--------------------------------------------------------------------+

static void lcd_test_text(void)
{
    DEBUG_PRINT("\r\n[LCD] 文字测试\r\n");

    lcd_set_back_color(LCD_BLACK);
    lcd_clear();

    /* 16x8 字体 — ASCII 可打印字符 */
    lcd_set_font(&LCD_Font_16x8);
    lcd_set_color(LCD_WHITE);
    lcd_show_string(0, 0, "LCD Font 16x8:");

    char buf[96];
    for (int i = 0; i < 95; i++) buf[i] = (char)(i + 32);
    buf[95] = '\0';
    lcd_set_color(LCD_CYAN);
    lcd_show_string(0, 20, buf);

    /* 12x6 字体 */
    lcd_set_font(&LCD_Font_12x6);
    lcd_set_color(LCD_WHITE);
    lcd_show_string(0, 50, "LCD Font 12x6:");
    lcd_set_color(LCD_YELLOW);
    lcd_show_string(0, 65, buf);

    /* 数字显示 */
    lcd_set_font(&LCD_Font_16x8);
    lcd_set_color(LCD_GREEN);
    lcd_show_string(0, 100, "Numbers:");
    lcd_show_num(120, 100, 12345, 5);
    lcd_show_num(200, 100, -999, 4);
    lcd_show_num(280, 100, 0, 1);

    delay_ms(2000);
}

//--------------------------------------------------------------------+
// 2D 图形测试
//--------------------------------------------------------------------+

static void lcd_test_graphics(void)
{
    DEBUG_PRINT("\r\n[LCD] 2D图形测试\r\n");

    lcd_set_back_color(LCD_BLACK);
    lcd_clear();

    /* 边框 */
    lcd_set_color(LCD_WHITE);
    lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    /* 直线 */
    lcd_set_color(LCD_RED);
    lcd_draw_line(50, 100, 350, 100);
    lcd_set_color(LCD_GREEN);
    lcd_draw_line(50, 120, 350, 200);
    lcd_set_color(LCD_BLUE);
    lcd_draw_line(50, 120, 350, 40);

    /* 矩形 */
    lcd_set_color(LCD_MAGENTA);
    lcd_draw_rect(400, 50, 150, 100);
    lcd_fill_rect(430, 70, 90, 60);

    /* 圆 */
    lcd_set_color(LCD_CYAN);
    lcd_draw_circle(200, 300, 80);
    lcd_set_color(LCD_YELLOW);
    lcd_fill_circle(500, 300, 60);

    /* 嵌套圆 */
    lcd_set_color(LCD_RED);
    for (int r = 20; r <= 80; r += 20) {
        lcd_draw_circle(650, 200, r);
    }

    delay_ms(3000);
}

//--------------------------------------------------------------------+
// 颜色填充测试
//--------------------------------------------------------------------+

static void lcd_test_color_bars(void)
{
    DEBUG_PRINT("\r\n[LCD] 颜色测试\r\n");

    lcd_set_back_color(LCD_BLACK);
    lcd_clear();

    lcd_font_t *f = &LCD_Font_12x6;
    lcd_set_font(f);

    typedef struct { uint32_t color; const char *name; } color_entry_t;
    color_entry_t colors[] = {
        { LCD_RED,       "RED" },
        { LCD_GREEN,     "GREEN" },
        { LCD_BLUE,      "BLUE" },
        { LCD_CYAN,      "CYAN" },
        { LCD_MAGENTA,   "MAGENTA" },
        { LCD_YELLOW,    "YELLOW" },
        { LCD_GREY,      "GREY" },
        { LIGHT_BLUE,    "L_BLUE" },
        { LIGHT_GREEN,   "L_GREEN" },
        { LIGHT_RED,     "L_RED" },
        { LIGHT_CYAN,    "L_CYAN" },
        { LIGHT_MAGENTA, "L_MAGENTA" },
        { LIGHT_YELLOW,  "L_YELLOW" },
        { LIGHT_GREY,    "L_GREY" },
        { DARK_BLUE,     "D_BLUE" },
        { DARK_GREEN,    "D_GREEN" },
        { DARK_RED,      "D_RED" },
        { DARK_CYAN,     "D_CYAN" },
        { DARK_MAGENTA,  "D_MAGENTA" },
        { DARK_YELLOW,   "D_YELLOW" },
        { DARK_GREY,     "D_GREY" },
    };

    int bar_h = 20;
    int bar_w = 400;
    int bar_x = 150;
    int gap   = 2;

    for (int i = 0; i < 21; i++) {
        int y = 10 + i * (bar_h + gap);
        lcd_set_color(colors[i].color);
        lcd_fill_rect(bar_x, y, bar_w, bar_h);

        lcd_set_color(LCD_WHITE);
        lcd_show_string(10, y + (bar_h - f->height) / 2, colors[i].name);
    }

    delay_ms(3000);
}

//--------------------------------------------------------------------+
// RGB 渐变测试
//--------------------------------------------------------------------+

static void lcd_test_gradient(void)
{
    DEBUG_PRINT("\r\n[LCD] RGB渐变测试\r\n");

    lcd_set_back_color(LCD_BLACK);
    lcd_clear();

    lcd_set_font(&LCD_Font_16x8);
    lcd_set_color(LCD_WHITE);
    lcd_show_string(10, 10, "RGB Gradient");

    int start_y = 50;
    int h = 100;
    int bar_w = LCD_WIDTH / 3;

    /* R 渐变 */
    for (int i = 0; i < 255; i++) {
        uint32_t c = (255 - i) << 16;
        int x = 0 + (i * bar_w) / 255;
        int w = bar_w / 255 + 1;
        lcd_set_color(c);
        lcd_fill_rect(x, start_y, w, h);
    }

    /* G 渐变 */
    for (int i = 0; i < 255; i++) {
        uint32_t c = (255 - i) << 8;
        int x = bar_w + (i * bar_w) / 255;
        int w = bar_w / 255 + 1;
        lcd_set_color(c);
        lcd_fill_rect(x, start_y, w, h);
    }

    /* B 渐变 */
    for (int i = 0; i < 255; i++) {
        uint32_t c = (255 - i);
        int x = 2 * bar_w + (i * bar_w) / 255;
        int w = bar_w / 255 + 1;
        lcd_set_color(c);
        lcd_fill_rect(x, start_y, w, h);
    }

    /* 文字标注 */
    lcd_set_color(LCD_RED);
    lcd_show_string(0, start_y + h + 10, "Red");
    lcd_set_color(LCD_GREEN);
    lcd_show_string(bar_w, start_y + h + 10, "Green");
    lcd_set_color(LCD_BLUE);
    lcd_show_string(2 * bar_w, start_y + h + 10, "Blue");

    delay_ms(3000);
}

//--------------------------------------------------------------------+
// 测试入口
//--------------------------------------------------------------------+

void lcd_test(void)
{
    DEBUG_PRINT("\r\n========================================\r\n");
    DEBUG_PRINT("  LCD 显示驱动测试\r\n");
    DEBUG_PRINT("  分辨率: %dx%d (ARGB8888)\r\n", LCD_WIDTH, LCD_HEIGHT);
    DEBUG_PRINT("========================================\r\n");

    /* 不调用 lcd_init(), 假设 main 已完成硬件初始化 */
    lcd_set_color(LCD_WHITE);
    lcd_set_back_color(LCD_BLACK);
    lcd_set_font(&LCD_Font_16x8);
    lcd_clear();

    lcd_test_clear();
    lcd_test_text();
    lcd_test_color_bars();
    lcd_test_gradient();
    lcd_test_graphics();

    /* 回到默认状态 */
    lcd_set_back_color(LCD_BLACK);
    lcd_set_color(LCD_WHITE);
    lcd_set_font(&LCD_Font_16x8);
    lcd_clear();
    lcd_show_string(10, 10, "LCD Test Done.");

    DEBUG_PRINT("\r\n[LCD] 测试完成\r\n");
}

#endif /* USE_BSP_UNIT_TESTING */
