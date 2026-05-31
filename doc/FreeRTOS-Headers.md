# FreeRTOS V11.3.0 头文件说明

> 芯片: STM32H743 (Cortex-M7), 编译器: ARM GCC, 移植: FreeRTOS\ARM_CM7

---

## 头文件分层关系

```
FreeRTOS.h          ← 必须第一个包含，包含 FreeRTOSConfig.h + stdint.h
├── projdefs.h      ← 基础类型和返回值定义 (TaskFunction_t, pdTRUE/pdFALSE)
├── portable.h      ← 引入 portmacro.h (平台相关)
│   └── portmacro.h ← Cortex-M7 特定: BASEPRI 临界区, PendSV 切换, CLZ 优先级
├── list.h          ← 内核内部双向链表 (调度器核心数据结构)
├── task.h          ← 任务管理 API
│   └── atomic.h    ← 原子操作 (关全局中断实现)
├── queue.h         ← 消息队列 API (也用于信号量/互斥量底层)
│   └── semphr.h    ← 信号量/互斥量 API (typedef QueueHandle_t)
├── timers.h        ← 软件定时器 API
├── event_groups.h  ← 事件组 API (依赖 timers.h)
├── stream_buffer.h ← 流式缓冲区 API
├── message_buffer.h← 消息缓冲区 API (基于 stream_buffer)
├── croutine.h      ← 协程 API (已废弃, 新项目不推荐)
├── stack_macros.h  ← 栈溢出检测宏 (内核内部使用)
├── newlib-freertos.h    ← Newlib C 库集成
├── picolibc-freertos.h  ← PicoLibC C 库集成
├── deprecated_definitions.h ← 旧版端口选择宏 (向下兼容)
├── mpu_prototypes.h      ← MPU 包装函数原型 (启用 MPU 时)
├── mpu_wrappers.h        ← MPU API 名称重映射 (启用 MPU 时)
└── mpu_syscall_numbers.h ← MPU 系统调用号 (启用 MPU 时)
```

---

## 1. 核心头文件

### FreeRTOS.h — 总入口

**职责**: 所有 FreeRTOS 头文件的前置依赖，定义了内核版本和基础配置。

**关键内容**:
- 包含 `<stddef.h>`, `<stdint.h>`
- 包含 `FreeRTOSConfig.h` (用户配置文件)
- 定义 `TICK_TYPE_WIDTH_16_BITS / 32_BITS / 64_BITS` 常量
- 定义 `ARRAY_LENGTH()` 等通用宏
- 声明 `xPortGetFreeHeapSize()`, `pvPortMalloc()`, `vPortFree()` 内存管理接口

**使用**: 任何 .c 文件使用 FreeRTOS API 前，第一行必须:
```c
#include "FreeRTOS.h"  // 必须在所有其他 FreeRTOS 头文件之前
#include "task.h"      // 然后才能包含具体功能头
```

**在本项目中的应用**:
```c
// platform/BSP/freertos_port/freertos_port.c
#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    taskDISABLE_INTERRUPTS();
    while (1) {}
}
```

---

### projdefs.h — 基础类型和返回值

**职责**: 定义内核通用的数据类型和返回值，不依赖其他 FreeRTOS 头文件。

**关键内容**:

| 定义 | 说明 |
|---|---|
| `TaskFunction_t` | 任务函数原型: `void func(void *arg)` |
| `pdTRUE` / `pdFALSE` | 布尔值 (1 / 0, 类型 BaseType_t) |
| `pdPASS` / `pdFAIL` | 函数返回值 (1 / 0) |
| `pdMS_TO_TICKS(ms)` | 毫秒 → 系统节拍数 |
| `pdTICKS_TO_MS(ticks)` | 系统节拍数 → 毫秒 |
| `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` | 内存不足错误码 (-1) |
| `errQUEUE_FULL` | 队满错误码 |

**示例**:
```c
// 将 500ms 等待时间转换为 tick
TickType_t xTicks = pdMS_TO_TICKS(500);
xQueueReceive(hQueue, &data, xTicks);  // 阻塞最多 500ms

// 检查返回值
if (xSemaphoreTake(hSem, portMAX_DELAY) == pdPASS) {
    // 成功获取
}
```

---

## 2. 任务管理

### task.h — 任务管理 API

**职责**: 创建、删除、调度和控制 RTOS 任务。

**关键类型和 API**:

| 函数 | 说明 |
|---|---|
| `xTaskCreate(task_func, name, stack, arg, prio, &handle)` | 动态创建任务 |
| `vTaskDelete(NULL)` | 删除自身任务 |
| `vTaskDelay(xTicks)` | 相对延时 (至少 N 个 tick) |
| `vTaskDelayUntil(&lastWake, xPeriod)` | 绝对延时 (固定频率) |
| `vTaskSuspend(hTask)` / `vTaskResume(hTask)` | 挂起/恢复 |
| `uxTaskGetStackHighWaterMark(hTask)` | 剩余栈空间 |
| `vTaskPrioritySet(hTask, prio)` | 修改优先级 |
| `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` | 进入/退出临界区 |
| `taskDISABLE_INTERRUPTS()` / `taskENABLE_INTERRUPTS()` | 关/开中断 |
| `taskYIELD()` | 手动触发任务切换 |
| `eTaskGetState(hTask)` | 获取任务状态 |
| `uxTaskGetNumberOfTasks()` | 当前任务总数 |
| `xTaskGetTickCount()` | 自调度器启动以来的 tick 数 |
| `xTaskNotify()` / `xTaskNotifyWait()` | 直接任务通知 (轻量级) |

**示例**:
```c
// 1. 创建任务
static void vLedTask(void *pvParameters) {
    uint32_t led_id = (uint32_t)pvParameters;
    for (;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_0 << led_id);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

TaskHandle_t hLed;
xTaskCreate(vLedTask, "LED", 256, (void *)1, 2, &hLed);

// 2. 固定频率执行 (用 vTaskDelayUntil)
TickType_t xLastWakeTime = xTaskGetTickCount();
for (;;) {
    do_work();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100)); // 精确 100ms 周期
}

// 3. 直接任务通知 (比信号量更快)
// 发送方 (ISR 中):
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xTaskNotifyFromISR(hTask, value, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

// 接收方 (任务中):
uint32_t ulNotificationValue;
xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY);
```

---

## 3. 同步与通信

### queue.h — 消息队列 API

**职责**: 提供任务间、中断与任务间的数据传递机制。信号量和互斥量底层也是队列 (queueQUEUE_TYPE_MUTEX 等)。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xQueueCreate(count, item_size)` | 创建队列 |
| `xQueueSend(hQueue, &item, timeout)` | 向队尾发送 (任务) |
| `xQueueReceive(hQueue, &item, timeout)` | 从队头接收 (任务) |
| `xQueueSendToFront(x,y,z)` / `xQueueSendToBack(x,y,z)` | 指定发送位置 |
| `xQueueOverwrite(hQueue, &item)` | 覆盖写入 (队列满时) |
| `xQueuePeek(hQueue, &item, timeout)` | 读取但不移除 |
| `xQueueSendFromISR(...)` / `xQueueReceiveFromISR(...)` | 中断安全版本 |
| `uxQueueMessagesWaiting(hQueue)` | 当前队列中消息数 |
| `uxQueueSpacesAvailable(hQueue)` | 剩余空间 |

**示例**:
```c
// 一个传递 float 的队列
QueueHandle_t xFloatQueue = xQueueCreate(10, sizeof(float));

// Task A: 发送
float temp = read_temperature();
xQueueSend(xFloatQueue, &temp, pdMS_TO_TICKS(100));

// Task B: 接收
float temp;
if (xQueueReceive(xFloatQueue, &temp, portMAX_DELAY) == pdPASS) {
    display(temp);
}

// ISR 中发送 (带上下文切换检查)
void ADC_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint16_t sample = ADC1->DR;
    xQueueSendFromISR(xQueue, &sample, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

---

### semphr.h — 信号量和互斥量 API

**职责**: 继承 `queue.h`，`SemaphoreHandle_t` 即 `QueueHandle_t`。信号量是长度为 N、项大小为 0 的队列。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xSemaphoreCreateBinary()` | 创建二值信号量 (默认为空) |
| `xSemaphoreCreateMutex()` | 创建互斥量 (带优先级继承) |
| `xSemaphoreCreateRecursiveMutex()` | 递归互斥量 (同一任务可多次获取) |
| `xSemaphoreCreateCounting(max, initial)` | 计数信号量 |
| `xSemaphoreTake(hSem, timeout)` | 获取 |
| `xSemaphoreGive(hSem)` | 释放 |
| `xSemaphoreTakeFromISR(x,y,z)` / `xSemaphoreGiveFromISR(x,y)` | 中断安全版本 |
| `xSemaphoreGetMutexHolder(hSem)` | 返回持有互斥量的任务 |

**二值信号量 vs 互斥量**:

| 特性 | 二值信号量 | 互斥量 |
|---|---|---|
| 用途 | 同步 (ISR → Task) | 资源互斥 (Task ↔ Task) |
| 优先级继承 | 无 | 有 (防优先级反转) |
| 谁获取谁释放 | 不限制 | 必须同一任务 |
| 中断中可用 | Take + Give | 只能 Task |
| 创建后状态 | 空 (需先 Give) | 满 (可直接 Take) |

**示例 (来自本项目 tlsf_port.c)**:
```c
// 创建互斥量
SemaphoreHandle_t g_mutex = xSemaphoreCreateMutex();

// 保护共享资源
#define TLSF_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
#define TLSF_UNLOCK() do { if (g_mutex) xSemaphoreGive(g_mutex); } while(0)

void *tlsf_port_malloc(size_t size, tlsf_port_pool_t pool) {
    TLSF_LOCK();
    void *p = tlsf_malloc(g_tlsf[pool], size);
    TLSF_UNLOCK();
    return p;
}
```

**二值信号量示例 (ISR 同步)**:
```c
// 创建
SemaphoreHandle_t xUartRxSem = xSemaphoreCreateBinary();

// ISR 中释放
void USART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xUartRxSem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// 任务中等待
void vUartTask(void *pv) {
    for (;;) {
        xSemaphoreTake(xUartRxSem, portMAX_DELAY);
        process_rx_byte();
    }
}
```

**计数信号量示例 (资源池管理)**:
```c
// 管理 4 个 DMA 通道
SemaphoreHandle_t xDmaSem = xSemaphoreCreateCounting(4, 4);

void vProcessTask(void *pv) {
    xSemaphoreTake(xDmaSem, portMAX_DELAY);  // 等待一个空闲通道
    do_dma_transfer();
    xSemaphoreGive(xDmaSem);                  // 释放通道
}
```

---

### event_groups.h — 事件组 API

**职责**: 一个 24 位或 32 位的位掩码，多个任务可等待不同的位组合。适合"等待多个事件满足"的场景。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xEventGroupCreate()` | 创建事件组 |
| `xEventGroupSetBits(x, bits)` | 置位 (任务/ISR) |
| `xEventGroupWaitBits(x, bits, clear, waitAll, timeout)` | 等待位 |
| `xEventGroupClearBits(x, bits)` | 清除指定位 |
| `xEventGroupSync(x, bits, allBits, timeout)` | 多任务同步点 |
| `xEventGroupGetBits(x)` | 读取当前位 |

**示例**:
```c
#define BIT_TEMP_OK    (1 << 0)
#define BIT_PRESSURE_OK (1 << 1)
#define BIT_WIFI_OK    (1 << 2)

EventGroupHandle_t xSensorEvents = xEventGroupCreate();

// Sensor Task A
void vTempTask(void *pv) {
    while (1) {
        float t = read_temp();
        if (t < 80.0f) xEventGroupSetBits(xSensorEvents, BIT_TEMP_OK);
        else           xEventGroupClearBits(xSensorEvents, BIT_TEMP_OK);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Monitor Task: 等待所有传感器就绪
void vMonitorTask(void *pv) {
    EventBits_t bits = xEventGroupWaitBits(
        xSensorEvents,
        BIT_TEMP_OK | BIT_PRESSURE_OK | BIT_WIFI_OK,  // 关心这些位
        pdTRUE,      // 读取后自动清除
        pdTRUE,      // 等待 ALL (不是 ANY)
        pdMS_TO_TICKS(5000)
    );
    if ((bits & (BIT_TEMP_OK | BIT_PRESSURE_OK | BIT_WIFI_OK)) ==
                (BIT_TEMP_OK | BIT_PRESSURE_OK | BIT_WIFI_OK)) {
        start_mission();  // 所有条件满足
    }
}
```

---

### timers.h — 软件定时器 API

**职责**: 不需要独立任务，由 FreeRTOS 定时器服务任务 (`xTimerCreateTimerTask`) 统一管理。可在指定时间后执行回调（one-shot）或周期性执行（auto-reload）。

**注意**: 本项目 `configUSE_TIMERS = 0`，软件定时器未启用。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xTimerCreate(name, period, autoReload, id, callback)` | 创建定时器 |
| `xTimerStart(hTimer, timeout)` | 启动 |
| `xTimerStop(hTimer, timeout)` | 停止 |
| `xTimerReset(hTimer, timeout)` | 重置 (从头计时) |
| `xTimerChangePeriod(hTimer, newPeriod, timeout)` | 修改周期 |
| `xTimerIsTimerActive(hTimer)` | 是否运行中 |

**示例**:
```c
// 一次性定时器: 2 秒后执行一次
TimerHandle_t xOneShot;

void vTimerCallback(TimerHandle_t xTimer) {
    // 定时器到期，执行操作 (此函数在定时器服务任务上下文中)
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_0);
}

void app_init(void) {
    xOneShot = xTimerCreate("Shot", pdMS_TO_TICKS(2000), pdFALSE, NULL, vTimerCallback);
    xTimerStart(xOneShot, 0);
}

// 周期性定时器
TimerHandle_t xPeriodic = xTimerCreate("Periodic", pdMS_TO_TICKS(100), pdTRUE, (void *)0, vTimerCallback);
xTimerStart(xPeriodic, 0);
```

---

## 4. 流 / 消息缓冲区

### stream_buffer.h — 流式缓冲区 API

**职责**: 连续字节流传输，专为中断→任务和核间通信优化。**单写者/单读者设计**，多写者需自行序列化。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xStreamBufferCreate(bufSize, triggerLevel)` | 创建 |
| `xStreamBufferSend(x, data, len, timeout)` | 发送 |
| `xStreamBufferReceive(x, data, len, timeout)` | 接收 |
| `xStreamBufferSendFromISR(x, data, len, &woken)` | ISR 发送 |
| `xStreamBufferReceiveFromISR(x, data, len, &woken)` | ISR 接收 |
| `xStreamBufferBytesAvailable(x)` | 可读字节数 |
| `xStreamBufferSpacesAvailable(x)` | 可写空间 |
| `xStreamBufferReset(x)` | 清空 |

**示例**:
```c
StreamBufferHandle_t xSensorStream;
char rx_buf[256];

void vSensorTask(void *pv) {
    xSensorStream = xStreamBufferCreate(1024, 32); // triggerLevel=32 表示至少 32 字节才唤醒
    for (;;) {
        size_t n = xStreamBufferReceive(xSensorStream, rx_buf, sizeof(rx_buf),
                                         pdMS_TO_TICKS(1000));
        if (n > 0) process_sensor_data(rx_buf, n);
    }
}

// ISR 中追加数据
void UART_Rx_ISR(void) {
    BaseType_t xWoken = pdFALSE;
    uint8_t byte = USART1->RDR;
    xStreamBufferSendFromISR(xSensorStream, &byte, 1, &xWoken);
    portYIELD_FROM_ISR(xWoken);
}
```

---

### message_buffer.h — 消息缓冲区 API

**职责**: 基于 `stream_buffer.h`，每次发送是一个可变长度的独立消息 (自动附加长度头)。适合"每帧数据是一个消息"的场景。

**关键 API**:

| 函数 | 说明 |
|---|---|
| `xMessageBufferCreate(bufSize)` | 创建 |
| `xMessageBufferSend(x, data, len, timeout)` | 发送一条消息 |
| `xMessageBufferReceive(x, data, maxLen, timeout)` | 接收一条消息 |
| `xMessageBufferSendFromISR(x, data, len, &woken)` | ISR 发送 |
| `xMessageBufferReceiveFromISR(x, data, maxLen, &woken)` | ISR 接收 |

**示例**:
```c
MessageBufferHandle_t xCmdBuffer;

// Producer
void vCmdProducerTask(void *pv) {
    char cmd[64];
    for (;;) {
        build_command(cmd);
        xMessageBufferSend(xCmdBuffer, cmd, strlen(cmd), portMAX_DELAY);
    }
}

// Consumer
void vCmdConsumerTask(void *pv) {
    char cmd[64];
    for (;;) {
        size_t n = xMessageBufferReceive(xCmdBuffer, cmd, sizeof(cmd), portMAX_DELAY);
        cmd[n] = '\0';
        execute_command(cmd);
    }
}
```

**StreamBuffer vs MessageBuffer**:

| | StreamBuffer | MessageBuffer |
|---|---|---|
| 数据形式 | 连续字节流 | 独立变长消息 |
| 边界 | 无 (应用层处理) | 自动保持 |
| 开销 | 无 | 每条消息附加 `sizeof(size_t)` 字节长度头 |
| 典型场景 | 传感器原始数据流 | 串口 AT 命令 |

---

## 5. 平台移植层

### portable.h — 移植抽象层

**职责**: FreeRTOS 内核与硬件之间的桥梁。包含 `portmacro.h`，定义对齐掩码等平台常量。应用代码通常不直接包含此文件。

**关键内容**:
- 包含 `deprecated_definitions.h` (向下兼容旧移植方式)
- 若 `portENTER_CRITICAL` 未定义，则 `#include "portmacro.h"`
- 定义 `portBYTE_ALIGNMENT_MASK` (本项目 `portBYTE_ALIGNMENT=8`, 掩码 `0x0007`)
- 声明 `pxPortInitialiseStack()` 内部函数

---

### portmacro.h (本项目 Cortex-M7 移植) — 平台特定定义

**在项目中的位置**: `platform/BSP/freertos_port/include/portmacro.h`

**核心内容**:

**类型定义**:
```c
typedef long             BaseType_t;    // 有符号基础类型
typedef unsigned long    UBaseType_t;   // 无符号基础类型
typedef uint32_t         TickType_t;    // 系统节拍计数 (configTICK_TYPE_WIDTH_IN_BITS=32)
#define portMAX_DELAY    ((TickType_t)0xFFFFFFFFUL)  // 无限等待
```

**临界区管理 (BASEPRI 方式, Cortex-M7 标准做法)**:
```c
// 进入临界区 — 保存当前 BASEPRI 并提升到 configMAX_SYSCALL_INTERRUPT_PRIORITY
#define portENTER_CRITICAL()   vPortEnterCritical()

// 退出临界区 — 恢复之前保存的 BASEPRI
#define portEXIT_CRITICAL()    vPortExitCritical()

// 关中断 — 提升 BASEPRI 到 configMAX_SYSCALL_INTERRUPT_PRIORITY (0x50)
#define portDISABLE_INTERRUPTS()  vPortRaiseBASEPRI()

// 开中断 — BASEPRI 设为 0 (不屏蔽任何中断)
#define portENABLE_INTERRUPTS()   vPortSetBASEPRI(0)
```

**BASEPRI 原理**: 写 BASEPRI 的值 N 屏蔽所有优先级 >= N 的中断。本项目中 `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4 = 0x50`，即屏蔽优先级 5~15 的中断，优先级 0~4 不被屏蔽（不能调用 FreeRTOS API）。

**任务切换 (PendSV)**:
```c
#define portYIELD()
    // 写 PendSV 挂起位触发 PendSV 异常
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
    // 数据 + 指令同步屏障
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");

// ISR 退出时判断是否需要切换
#define portEND_SWITCHING_ISR(xSwitchRequired)
    if (xSwitchRequired != pdFALSE) { portYIELD(); }

// ISR 中调用: 请求切换并在退出时执行
#define portYIELD_FROM_ISR(x)  portEND_SWITCHING_ISR(x)
```

**硬件优化: 优先级选择 (Cortex-M CLZ 指令)**:
```c
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
// 用 CLZ (Count Leading Zeros) 快速找到最高优先级就绪任务
#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities)
    uxTopPriority = (31UL - ucPortCountLeadingZeros(uxReadyPriorities))
#endif
```

**ISR 上下文检测**:
```c
portFORCE_INLINE static BaseType_t xPortIsInsideInterrupt(void) {
    uint32_t ulCurrentInterrupt;
    __asm volatile("mrs %0, ipsr" : "=r"(ulCurrentInterrupt)::"memory");
    return (ulCurrentInterrupt != 0) ? pdTRUE : pdFALSE;
}
```
读取 IPSR (Interrupt Program Status Register): 非 0 表示当前在 ISR 中。

---

### FreeRTOSConfig.h (本项目配置) — 用户配置文件

**在项目中的位置**: `platform/BSP/freertos_port/include/FreeRTOSConfig.h`

**本项目关键配置**:
```c
// 时钟
#define configCPU_CLOCK_HZ           SystemCoreClock  // 240MHz (HCLK)
#define configTICK_RATE_HZ           1000             // 1ms tick

// 调度器
#define configUSE_PREEMPTION         1   // 抢占式
#define configUSE_TIME_SLICING       1   // 同优先级时间片轮转

// 优先级 (STM32H7 4 位优先级: 0-15)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  0x50   // NVIC 优先级 0-4 不能调用 API
#define configPRIO_BITS              4

// 任务
#define configMAX_PRIORITIES         32
#define configMINIMAL_STACK_SIZE     128

// 内存
#define configSUPPORT_DYNAMIC_ALLOCATION  1  // 动态分配
#define configSUPPORT_STATIC_ALLOCATION   0  // 不使用静态分配

// 特性
#define configUSE_MUTEXES            1   // 启用互斥量
#define configUSE_TASK_NOTIFICATIONS 1   // 启用直接任务通知
#define configUSE_TASK_FPU_SUPPORT   1   // 自动保存/恢复 FPU 寄存器
#define configCHECK_FOR_STACK_OVERFLOW 2 // 栈溢出检测 (方法2)
#define configUSE_TIMERS             0   // 未使用软件定时器

// Newlib C 库集成
#include "newlib-freertos.h"

// 中断向量映射
#define vPortSVCHandler             SVC_Handler
#define xPortPendSVHandler          PendSV_Handler
#define xPortSysTickHandler         SysTick_Handler
```

---

## 6. 内核内部头文件 (应用层一般不直接使用)

### list.h — 双向链表

调度器核心数据结构。所有 FreeRTOS 对象 (任务、队列、信号量、定时器) 都通过链表管理。

**关键类型**:
```c
struct xLIST_ITEM {
    TickType_t xItemValue;          // 排序键值 (如任务唤醒时间)
    struct xLIST_ITEM *pxNext;      // 后继指针
    struct xLIST_ITEM *pxPrevious;  // 前驱指针
    void *pvOwner;                  // 所属对象 (TCB / Queue / ...)
    void *pvContainer;              // 所在 List_t
};
typedef struct xLIST_ITEM ListItem_t;
```

**使用场景**: 就绪列表、延迟列表、挂起列表都是 `List_t`。应用层一般不直接操作。

---

### atomic.h — 原子操作

通过关全局中断实现的可移植原子操作。

**说明**: Cortex-M7 有 `LDREX/STREX` 指令可实现无锁原子操作，但 FreeRTOS 的通用实现使用 `portSET_INTERRUPT_MASK_FROM_ISR() / portCLEAR_INTERRUPT_MASK_FROM_ISR()` 关中断。仅在 `portHAS_NESTED_INTERRUPTS=1` (支持中断嵌套) 的移植中，ISR 中也可使用。

---

### stack_macros.h — 栈溢出检测

当 `configCHECK_FOR_STACK_OVERFLOW != 0` 时，任务切换时自动检查栈是否溢出。

- **方法1** (`configCHECK_FOR_STACK_OVERFLOW = 1`): 检查当前 SP 是否越界
- **方法2** (`configCHECK_FOR_STACK_OVERFLOW = 2`): 方法1 + 检查栈底几个字节是否被改写（栈初始化时填入的魔数 0xA5A5A5A5U）

本项目使用**方法2**，溢出时调用 `vApplicationStackOverflowHook()`:
```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    taskDISABLE_INTERRUPTS();
    while (1) {}  // 挂起调试
}
```

---

## 7. C 库集成头文件

### newlib-freertos.h — Newlib 集成

newlib 的 `malloc()` / `free()` 是全局的，多线程时需要锁保护。此文件:

1. 用 FreeRTOS 互斥量实现 `__malloc_lock()` / `__malloc_unlock()`
2. 为每个任务分配独立的 `struct _reent`，实现线程安全的 `errno`

本项目通过 `--wrap` 链接器标志将 `malloc` / `free` 重定向到 TLSF:
```cmake
# CMakeLists.txt
add_link_options(-Wl,--wrap,malloc -Wl,--wrap,free -Wl,--wrap,realloc)
```

因此 newlib-freertos.h 的 `__malloc_lock` 对 TLSF 也有效。

---

### picolibc-freertos.h — PicoLibC 集成

为 picolibc 提供 TLS (Thread-Local Storage) 支持，在任务切换时自动切换 `__thread` 变量上下文。本项目使用 newlib，此文件不生效。

---

## 8. MPU 相关头文件 (本项目中未使用)

这三个头文件仅在启用 MPU (Memory Protection Unit) 时生效，通过 `configUSE_MPU_WRAPPERS_V1` 控制:

| 文件 | 作用 |
|---|---|
| `mpu_prototypes.h` | 定义 `MPU_xTaskCreate()` 等包装函数原型 |
| `mpu_wrappers.h` | `#define vTaskDelay MPU_vTaskDelay` 将 API 映射到 MPU 版本 |
| `mpu_syscall_numbers.h` | SVC 系统调用号枚举 (MPU 保护的任务需通过 SVC 进入内核) |

---

## 9. 废弃头文件

### croutine.h — 协程 (已废弃)

无栈协程，比任务更省内存。V11.x 已不推荐使用，新项目应使用任务。本项目未使用。

### deprecated_definitions.h — 旧版移植方式

旧版 FreeRTOS 通过 `#define __ARM_CM3` / `__ARM_CM4F` 等宏来找到 `portmacro.h`。V11.x 改用 include path 方式，此文件保留向下兼容。

---

## include 规则速查

| .c 文件需求 | 需要的 #include (按顺序) |
|---|---|
| 创建任务 | `FreeRTOS.h` → `task.h` |
| 使用队列 | `FreeRTOS.h` → `queue.h` |
| 使用信号量/互斥量 | `FreeRTOS.h` → `semphr.h` |
| 使用软件定时器 | `FreeRTOS.h` → `timers.h` |
| 使用事件组 | `FreeRTOS.h` → `event_groups.h` |
| 使用流/消息缓冲区 | `FreeRTOS.h` → `stream_buffer.h` / `message_buffer.h` |
| 使用原子操作 | `FreeRTOS.h` → `atomic.h` |
| 全部功能 | `FreeRTOS.h` → `task.h` → 其余按需 |

**CRITICAL**: `FreeRTOS.h` 必须总是在任何其他 FreeRTOS 头文件之前。

---

## 本项目实际使用情况

基于 `FreeRTOSConfig.h` 配置，本项目实际启用的功能:

| 功能 | 启用 | 相关头文件 | 使用位置 |
|---|---|---|---|
| 任务管理 | 是 | `task.h` | 全局 |
| 互斥量 | 是 | `semphr.h` | `tlsf_port.c`, `sd.c` |
| 直接任务通知 | 是 | `task.h` | (可选, 未实际使用) |
| 二值信号量 | 是 | `semphr.h` | (可选) |
| 队列 | 是 | `queue.h` | (可选) |
| 递归互斥量 | 否 | — | — |
| 计数信号量 | 否 | — | — |
| 软件定时器 | 否 | — | — |
| 事件组 | 否 | — | — |
| 队列集 | 否 | — | — |
| 静态分配 | 否 | — | — |
