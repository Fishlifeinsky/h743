#ifndef BSP_UART_H_
#define BSP_UART_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 串口数量 — 由 USED_UARTS 自动计算
//--------------------------------------------------------------------+
#define UART_COUNT (sizeof((UART_HandleTypeDef *[]) { USED_UARTS }) / sizeof(UART_HandleTypeDef *))

// DMA 发送阈值: 小于此值使用轮询发送
#define UART_TX_DMA_THRESHOLD 32

// 串口句柄数组 (由 USED_UARTS 宏填充)
extern UART_HandleTypeDef *const s_uart_handles[];

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

// 初始化串口硬件 (启动 RXNE 中断, RTOS 资源在 uart_rtos_init 中创建)
// 调用前需确保 CubeMX 的 HAL_UART_MspInit 已完成
void uart_init(void);

// 创建 RTOS 资源 (互斥量/流缓冲区/轮询任务, 调度器启动后调用)
void uart_rtos_init(void);

// 注册 VFS 节点 "/dev/uart"
void uart_vfs_init(void);

// ISR 入口 (由 USARTx_IRQHandler 调用, 处理 LL 字节接收)
void uart_isr(int id);

// 发送数据 (DMA 或轮询, 自动选择). 内部加锁
size_t uart_write(int id, const uint8_t *data, size_t size);

// 阻塞接收 (FreeRTOS 下用流缓冲区阻塞, 裸机下轮询)
size_t uart_read(int id, uint8_t *data, size_t max_size, uint32_t timeout_ms);

// 非阻塞接收 (立即返回已有数据)
size_t uart_poll(int id, uint8_t *data, size_t max_size);

// 串口加锁/解锁 (供 VFS 或外部多任务场景)
void uart_lock(int id);
void uart_unlock(int id);

// 串口是否就绪
bool uart_is_ready(int id);

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void uart_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H_ */
