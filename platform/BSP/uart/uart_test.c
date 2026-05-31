#include "uart.h"
#include <string.h>
#include "usart.h"

#if USE_BSP_UNIT_TESTING

//--------------------------------------------------------------------+
// 测试缓冲区
//--------------------------------------------------------------------+

static uint8_t g_tx_buf[128];
static uint8_t g_rx_buf[32];

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void uart_test(void)
{
    DEBUG_PRINT("\r\n========== UART 单元测试 ==========\r\n");

    int count = (int)UART_COUNT;
    DEBUG_PRINT("串口数量: %d\r\n", count);

    const char *overall = "PASS";

    for (int id = 0; id < count; id++) {

        DEBUG_PRINT("\r\n--- COM%d ---\r\n", id);

        // === 1. 就绪检查 ===
        if (!uart_is_ready(id)) {
            DEBUG_PRINT("  COM%d not ready\r\n", id);
            overall = "FAIL";
            continue;
        }
        DEBUG_PRINT("  就绪: OK\r\n");

        // === 2. 轮询空缓冲区 ===
        size_t n = uart_poll(id, g_rx_buf, sizeof(g_rx_buf));
        DEBUG_PRINT("  空轮询: %zu 字节\r\n", n);

        // === 3. 短数据轮询发送 (不使用 DMA) ===
        const char *msg = "TEST\r\n";
        size_t  wsize  = strlen(msg);
        size_t w = uart_write(id, (uint8_t *)msg, wsize);
        if (w == wsize) {
            DEBUG_PRINT("  短发送: OK (%zu/%zu)\r\n", w, wsize);
        } else {
            DEBUG_PRINT("  短发送: FAIL (%zu/%zu)\r\n", w, wsize);
            overall = "FAIL";
        }

        // === 4. 长数据 DMA 发送 ===
        memset(g_tx_buf, 0xAA, sizeof(g_tx_buf));
        w = uart_write(id, g_tx_buf, sizeof(g_tx_buf));
        if (w == sizeof(g_tx_buf)) {
            DEBUG_PRINT("  DMA 发送: OK (%zu/%zu)\r\n", w, sizeof(g_tx_buf));
        } else {
            DEBUG_PRINT("  DMA 发送: FAIL (%zu/%zu)\r\n", w, sizeof(g_tx_buf));
            overall = "FAIL";
        }

        // === 5. 超时读取 (没有外部设备发送数据, 应该超时返回 0) ===
        size_t r = uart_read(id, g_rx_buf, sizeof(g_rx_buf), 10000);
        DEBUG_PRINT("  超时读取: %zu 字节 (预期=0 无外部数据)\r\n", r);

        // === 6. 互斥量测试 ===
        uart_lock(id);
        DEBUG_PRINT("  互斥量: lock OK\r\n");
        n = uart_poll(id, g_rx_buf, sizeof(g_rx_buf));
        DEBUG_PRINT("  加锁后轮询: %zu 字节\r\n", n);
        uart_unlock(id);
        DEBUG_PRINT("  互斥量: unlock OK\r\n");
    }

    // === 报告 ===
    DEBUG_PRINT("\r\n");
    DEBUG_PRINT("┌──────────────────────────────┐\r\n");
    DEBUG_PRINT("│     UART 单元测试报告         │\r\n");
    DEBUG_PRINT("├──────────────────────────────┤\r\n");
    DEBUG_PRINT("│ 串口数量: %d                   │\r\n", count);
    DEBUG_PRINT("│ 结果:     %-13s │\r\n", overall);
    DEBUG_PRINT("│                              │\r\n");
    DEBUG_PRINT("│ 注: 需外部硬件回环             │\r\n");
    DEBUG_PRINT("│ 才能测试 RX 功能               │\r\n");
    DEBUG_PRINT("└──────────────────────────────┘\r\n");
}

#endif // USE_BSP_UNIT_TESTING
