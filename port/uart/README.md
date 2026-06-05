# UART 串口封装模块

基于 LL 库的 RXNE 逐字节中断接收 + FreeRTOS StreamBuffer，提供统一的串口读写接口。

## 概述

| | 说明 |
|---|---|
| 接收方式 | LL `USART_ISR_RXNE_RXFNE` 逐字节中断 |
| 接收缓冲 | 内部 `raw_ring` (512B) + FreeRTOS StreamBuffer (2KB) |
| 发送方式 | HAL 轮询或 DMA (`size < 32` 轮询，否则 DMA) |
| 任务模型 | 每串口一个 `uart_poll_task`，将 `raw_ring` 批处理到 StreamBuffer |
| 多任务安全 | 每串口独立互斥量保护 TX |
| VFS 路径 | `/dev/uart/COM{N}` |

## 文件结构

```
BSP/uart/
├── include/
│   └── uart.h              # 公共头文件
├── uart.c                  # 主实现 (LL RXNE ISR / StreamBuffer / 轮询任务)
├── uart_vfs.c              # VFS 节点 "/dev/uart"
└── README.md
```

## 配置

在 `BSP/config.h` 中定义已使用的串口：

```c
#define USED_UARTS  &huart1
```

串口 ID 按数组顺序分配：

| USED_UARTS 位置 | ID | VFS 路径 |
|---|---|---|
| 第1个 | 0 | `/dev/uart/COM0` |

## 调用顺序

```c
// main() 中（调度器启动前）— 仅硬件初始化
MX_USART1_UART_Init();   // CubeMX 配置
uart_init();             // 清零上下文、启用 RXNE 中断
uart_vfs_init();         // 注册 /dev/uart

// init_task 中（调度器启动后）— RTOS 资源初始化
uart_rtos_init();        // 创建互斥量、StreamBuffer、轮询任务
```

**注意**：`uart_init()` 与 `uart_rtos_init()` 必须分开调用。前者在 `main()` 中执行（调度器启动前），后者在 `init_task` 中执行（调度器启动后）。若在调度器启动前调用 `xSemaphoreCreateMutexStatic` 等 FreeRTOS API，会导致 `BASEPRI` 被永久置位，中断卡死。

## API

### 直接调用

```c
// 发送（内部自动加锁）
size_t n = uart_write(0, data, len);

// 阻塞接收（FreeRTOS: StreamBuffer 原生阻塞；裸机: system_delay_ms 轮询）
size_t n = uart_read(0, buf, sizeof(buf), 100);

// 非阻塞接收
size_t n = uart_poll(0, buf, sizeof(buf));

// 多任务保护（供 VFS 或外部多任务场景）
uart_lock(0);
uart_write(0, data, len);
uart_unlock(0);

// 状态查询
bool ready = uart_is_ready(0);
```

### VFS 文件 I/O

注册后通过标准文件 I/O 访问：

```c
int fd = open("/dev/uart/COM0", O_RDWR);
if (fd >= 0) {
    write(fd, "hello\r\n", 7);
    char buf[128];
    int n = read(fd, buf, sizeof(buf));
    close(fd);
}
```

## 数据流

```
┌─────────────┐   ISR    ┌───────────┐  poll_task  ┌─────────────┐  uart_read  ┌──────┐
│ USART RXNE  │─────────→│ raw_ring  │────────────→│ StreamBuffer│────────────→│ 用户 │
│  (LL 逐字节)│          │ (512B)    │  (批处理)   │  (2KB)      │  (阻塞)    │ Task │
└─────────────┘          └───────────┘             └─────────────┘             └──────┘
```

1. **ISR** (`uart_isr`): 每收到一个字节，读入 `raw_ring`；唤醒 `uart_poll_task`
2. **轮询任务** (`uart_poll_task`): 批处理 `raw_ring` → `StreamBuffer`（10ms 静默超时或缓冲区满时刷新）
3. **应用读取** (`uart_read`): 从 `StreamBuffer` 阻塞读取

## FreeRTOS vs 裸机

| | FreeRTOS | 裸机 |
|---|---|---|
| 接收缓冲 | `raw_ring` + `StreamBuffer` | 仅 `raw_ring` |
| 阻塞方式 | `xStreamBufferReceive(timeout)` | `system_delay_ms` 轮询 |
| 回调通知 | `vTaskNotifyGiveFromISR` + `portYIELD_FROM_ISR` | 无 |
| 互斥量 | `SemaphoreHandle_t`（每串口独立） | 无 |
| 轮询任务 | `uart_poll_task`（每串口一个） | 无 |

## 注意事项

- `uart_init()` 仅做硬件初始化，**不**创建任何 FreeRTOS 对象
- `uart_rtos_init()` 必须在 `vTaskStartScheduler()` 之后调用
- `uart_write()` 内部使用 `system_delay_ms()` 等待 HAL 状态就绪，在调度器启动前后均可正常工作
