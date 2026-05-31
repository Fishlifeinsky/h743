#include "uart.h"
#include "system_delay.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "stm32h7xx_ll_usart.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"
#endif

//--------------------------------------------------------------------+
// 内部宏
//--------------------------------------------------------------------+

#define UART_MAX_COUNT          4
#define UART_RAW_RING_SIZE      512   // ISR → 轮询任务中间缓冲区 (2 的幂)
#define UART_RING_MASK          (UART_RAW_RING_SIZE - 1)
#define UART_BATCH_SIZE         128   // 轮询任务本地批处理缓冲区
#define UART_STREAM_BUF_SIZE    2048  // 应用层流缓冲区大小
#define UART_POLL_TASK_STACK    256   // 轮询任务栈大小 (words)
#define UART_POLL_TASK_PRIO     2     // 轮询任务优先级
#define UART_FLUSH_TIMEOUT_MS   10    // 静默超时后刷入流缓冲区

//--------------------------------------------------------------------+
// 外设上下文
//--------------------------------------------------------------------+

typedef struct {
    UART_HandleTypeDef  *huart;
    USART_TypeDef       *instance;  // LL 寄存器基址

    // 原始接收环形缓冲区 (ISR 写入, 轮询任务/应用读取)
    uint8_t              raw_ring[UART_RAW_RING_SIZE];
    volatile uint32_t    raw_head;  // ISR 递增加入
    uint32_t             raw_tail;  // 任务递增取出

#if BSP_FREERTOS_ENABLED
    SemaphoreHandle_t    mutex;     // TX 序列化
    StreamBufferHandle_t stream;    // 应用层流缓冲区
    TaskHandle_t         poll_task;
#else
    uint32_t             head;      // 裸机: raw_ring 的应用层读索引
    uint32_t             tail;
#endif
} uart_ctx_t;

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

UART_HandleTypeDef *const s_uart_handles[] = { USED_UARTS };

static uart_ctx_t g_ctx[UART_MAX_COUNT];

#if BSP_FREERTOS_ENABLED
static StaticSemaphore_t    g_mutex_buf[UART_MAX_COUNT];
static StaticStreamBuffer_t g_stream_buf[UART_MAX_COUNT];
static uint8_t              g_stream_storage[UART_MAX_COUNT][UART_STREAM_BUF_SIZE];
static StaticTask_t         g_poll_task_tcb[UART_MAX_COUNT];
static StackType_t          g_poll_task_stack[UART_MAX_COUNT][UART_POLL_TASK_STACK];
#endif

//--------------------------------------------------------------------+
// ISR: per-character RX (由 USARTx_IRQHandler 调用, 在 HAL_UART_IRQHandler 之前)
//--------------------------------------------------------------------+

void uart_isr(int id)
{
    if (id < 0 || id >= (int)UART_COUNT) return;
    uart_ctx_t *ctx = &g_ctx[id];
    USART_TypeDef *usart = ctx->instance;
    if (!usart) return;

    uint32_t isr = usart->ISR;

    // 1. 错误标志 (ORE/FE/PE) — 必须先读 RDR 再通过 ICR 清除, 否则锁死 UART
    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_PE)) {
        volatile uint32_t dummy __attribute__((unused)) = usart->RDR;
        if (isr & USART_ISR_ORE) usart->ICR = USART_ICR_ORECF;
        if (isr & USART_ISR_FE)  usart->ICR = USART_ICR_FECF;
        if (isr & USART_ISR_PE)  usart->ICR = USART_ICR_PECF;
    }

    // 2. RXNE: 逐字节读入 raw_ring
    while (usart->ISR & USART_ISR_RXNE_RXFNE) {
        uint8_t byte = (uint8_t)(usart->RDR & 0xFF);

        if (ctx->raw_head - ctx->raw_tail < UART_RAW_RING_SIZE) {
            ctx->raw_ring[ctx->raw_head & UART_RING_MASK] = byte;
            ctx->raw_head++;
        }
        // 环形缓冲区满则丢弃 (不阻塞 ISR)
    }

#if BSP_FREERTOS_ENABLED
    // 3. 唤醒轮询任务
    if (ctx->poll_task) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(ctx->poll_task, &woken);
        portYIELD_FROM_ISR(woken);
    }
#endif
}

//--------------------------------------------------------------------+
// FreeRTOS 轮询任务: 从 raw_ring 批处理 → 流缓冲区 (10ms 静默超时)
//--------------------------------------------------------------------+

#if BSP_FREERTOS_ENABLED
static void uart_poll_task(void *pvParameters)
{
    int id = (int)(uintptr_t)pvParameters;
    uart_ctx_t *ctx = &g_ctx[id];

    uint8_t  batch[UART_BATCH_SIZE];
    size_t   batch_len  = 0;
    TickType_t last_rx  = 0;
    bool     collecting = false;

    for (;;) {
        // 1. 从 raw_ring 搬运到 batch
        while (ctx->raw_tail != ctx->raw_head && batch_len < sizeof(batch)) {
            batch[batch_len++] = ctx->raw_ring[ctx->raw_tail & UART_RING_MASK];
            ctx->raw_tail++;
            last_rx    = xTaskGetTickCount();
            collecting = true;
        }

        // 2. 判断是否刷入流缓冲区
        if (collecting) {
            bool ring_empty = (ctx->raw_tail == ctx->raw_head);
            bool full       = (batch_len >= sizeof(batch));
            bool timeout    = (xTaskGetTickCount() - last_rx) >= pdMS_TO_TICKS(UART_FLUSH_TIMEOUT_MS);

            if (full || timeout || ring_empty) {
                if (batch_len > 0) {
                    xStreamBufferSend(ctx->stream, batch, batch_len, 0);
                    batch_len = 0;
                }
                if (ring_empty) {
                    collecting = false;
                } else {
                    last_rx = xTaskGetTickCount();
                }
            }
        }

        // 3. 等待 ISR 通知或 1ms 超时
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
    }
}
#endif

//--------------------------------------------------------------------+
// 外部接口: 初始化
//--------------------------------------------------------------------+

void uart_init(void)
{
    int count = (int)UART_COUNT;

    for (int i = 0; i < count; i++) {
        uart_ctx_t *ctx = &g_ctx[i];
        memset(ctx, 0, sizeof(*ctx));

        ctx->huart    = s_uart_handles[i];
        ctx->instance = ctx->huart->Instance;
        USART_TypeDef *usart = ctx->instance;

        // 1. 停止可能正在运行的 HAL DMA RX
        HAL_UART_AbortReceive(ctx->huart);

        // 2. 清除所有 UART 中断挂起标志
        usart->ICR = 0xFFFFFFFFU;

        // 3. 启用 RXNE 中断 (LL 库: RXNEIE_RXFNEIE)
        LL_USART_EnableIT_RXNE_RXFNE(usart);

        DEBUG_PRINT("UART[%d]: init ok (LL RXNE)\r\n", i);
    }
    DEBUG_PRINT("UART: init %d port(s)\r\n", count);
}

void uart_rtos_init(void)
{
#if BSP_FREERTOS_ENABLED
    int count = (int)UART_COUNT;

    for (int i = 0; i < count; i++) {
        uart_ctx_t *ctx = &g_ctx[i];

        ctx->mutex = xSemaphoreCreateMutexStatic(&g_mutex_buf[i]);

        ctx->stream = xStreamBufferCreateStatic(UART_STREAM_BUF_SIZE, 1,
                                                g_stream_storage[i],
                                                &g_stream_buf[i]);

        char name[16];
        snprintf(name, sizeof(name), "uart_poll_%d", i);
        ctx->poll_task = xTaskCreateStatic(
            uart_poll_task, name,
            UART_POLL_TASK_STACK, (void *)(uintptr_t)i,
            UART_POLL_TASK_PRIO,
            g_poll_task_stack[i], &g_poll_task_tcb[i]);

        DEBUG_PRINT("UART[%d]: rtos init ok\r\n", i);
    }
    DEBUG_PRINT("UART: rtos init %d port(s)\r\n", count);
#endif
}

//--------------------------------------------------------------------+
// 外部接口: 发送 (TX 路径不变)
//--------------------------------------------------------------------+

size_t uart_write(int id, const uint8_t *data, size_t size)
{
    if (id < 0 || id >= (int)UART_COUNT || !data || size == 0) return 0;
    uart_ctx_t *ctx = &g_ctx[id];
    UART_HandleTypeDef *huart = ctx->huart;

#if BSP_FREERTOS_ENABLED
    if (ctx->mutex) {
        xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    }
#endif

    size_t sent = 0;

    if (size < UART_TX_DMA_THRESHOLD) {
        while (huart->gState != HAL_UART_STATE_READY) { system_delay_ms(1); }
        if (HAL_UART_Transmit(huart, (uint8_t *)data, (uint16_t)size, 5000) == HAL_OK) {
            sent = size;
        }
    } else {
        while (huart->gState != HAL_UART_STATE_READY) { system_delay_ms(1); }
        SCB_CleanDCache_by_Addr((uint32_t *)data, (int32_t)size);
        if (HAL_UART_Transmit_DMA(huart, (uint8_t *)data, (uint16_t)size) == HAL_OK) {
            while (huart->gState != HAL_UART_STATE_READY) { system_delay_ms(1); }
            sent = size;
        }
    }

#if BSP_FREERTOS_ENABLED
    if (ctx->mutex) {
        xSemaphoreGive(ctx->mutex);
    }
#endif

    return sent;
}

//--------------------------------------------------------------------+
// 外部接口: 阻塞接收
//--------------------------------------------------------------------+

size_t uart_read(int id, uint8_t *data, size_t max_size, uint32_t timeout_ms)
{
    if (id < 0 || id >= (int)UART_COUNT || !data || max_size == 0) return 0;
    uart_ctx_t *ctx = &g_ctx[id];

#if BSP_FREERTOS_ENABLED
    if (!ctx->stream) return 0;
    return xStreamBufferReceive(ctx->stream, (void *)data, max_size,
                                pdMS_TO_TICKS(timeout_ms));
#else
    size_t   total   = 0;
    uint32_t elapsed = 0;
    while (total < max_size) {
        if (ctx->tail != ctx->raw_head) {
            data[total++] = ctx->raw_ring[ctx->tail & UART_RING_MASK];
            ctx->tail++;
        } else {
            if (timeout_ms == 0) break;
            system_delay_ms(1);
            if (++elapsed >= timeout_ms) break;
        }
    }
    return total;
#endif
}

//--------------------------------------------------------------------+
// 外部接口: 非阻塞接收
//--------------------------------------------------------------------+

size_t uart_poll(int id, uint8_t *data, size_t max_size)
{
    if (id < 0 || id >= (int)UART_COUNT || !data || max_size == 0) return 0;
    uart_ctx_t *ctx = &g_ctx[id];

#if BSP_FREERTOS_ENABLED
    if (!ctx->stream) return 0;
    return xStreamBufferReceive(ctx->stream, (void *)data, max_size, 0);
#else
    return uart_read(id, data, max_size, 0);
#endif
}

//--------------------------------------------------------------------+
// 外部接口: 加锁/解锁 (VFS / 多任务场景)
//--------------------------------------------------------------------+

void uart_lock(int id)
{
    if (id < 0 || id >= (int)UART_COUNT) return;
#if BSP_FREERTOS_ENABLED
    if (g_ctx[id].mutex) {
        xSemaphoreTake(g_ctx[id].mutex, portMAX_DELAY);
    }
#endif
}

void uart_unlock(int id)
{
    if (id < 0 || id >= (int)UART_COUNT) return;
#if BSP_FREERTOS_ENABLED
    if (g_ctx[id].mutex) {
        xSemaphoreGive(g_ctx[id].mutex);
    }
#endif
}

//--------------------------------------------------------------------+
// 外部接口: 状态查询
//--------------------------------------------------------------------+

bool uart_is_ready(int id)
{
    return (id >= 0 && id < (int)UART_COUNT && g_ctx[id].huart != NULL);
}
