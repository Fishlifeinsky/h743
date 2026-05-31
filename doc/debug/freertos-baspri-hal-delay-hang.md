# HAL_Delay 在 FreeRTOS 启用后卡死问题

## 版本信息

- 芯片: STM32H743IIT6 (Cortex-M7)
- FreeRTOS: V11.3.0 (GCC/ARM_CM7/r0p1 port)
- HAL 时基: TIM7 (非 SysTick)
- 现象触发条件: `BSP_FREERTOS_ENABLED=1`，调度器尚未启动

## 现象

启用 FreeRTOS 编译后，`HAL_Delay()` 永久卡死。此时调度器尚未启动（`vTaskStartScheduler` 未调用），但 `tlsf_port_init()` 和 `sd_init()` 已通过 `xSemaphoreCreateMutexStatic()` 创建了 FreeRTOS 互斥锁。

串口输出 `HAL_GetTick()` 卡在 29 不再增长。

## 调试过程

### 第一步：确认 tick 是否在增长

在每个初始化函数之后记录 `HAL_GetTick()`，发现 tick 从 0 正常增长到 29（TIM7 ISR 在初始化阶段正常工作），但之后不再变化。

这说明 TIM7 ISR 在某个时刻**停止被执行**了。

### 第二步：排除 SysTick 干扰

怀疑 FreeRTOS 的 SysTick 配置干扰了时基。读取 SysTick 控制寄存器：

```
SysTick CTRL=00000000 LOAD=00000000 VAL=00000000
```

SysTick 未使能，排除。

### 第三步：确认 TIM7 硬件状态

```
TIM7 CR1=00000001 (计数器在跑) DIER=00000001 (中断使能) SR=00000001 (更新标志置位)
```

TIM7 硬件一切正常：计数器在跑、中断使能、更新事件已发生。但 ISR 没有被 CPU 响应。

### 第四步：检查 CPU 中断屏蔽状态

```
PRIMASK=0 BASEPRI=80 FAULTMASK=0
TIM7 NVIC enable=1 pending=1
LTDC NVIC enable=1 pending=0
```

**关键发现：BASEPRI = 0x80 (优先级 8)**

在 Cortex-M7 中，BASEPRI 屏蔽所有**优先级数值 ≥ BASEPRI 值**的中断：

```
BASEPRI = 0x80  →  屏蔽优先级 8,9,10,...,15
TIM7     = 15   →  15 ≥ 8，被屏蔽
NVIC pending = 1 →  中断已触发，但 CPU 永远不响应
```

### 第五步：追踪 BASEPRI 来源

FreeRTOS 临界区宏展开：

```c
// FreeRTOSConfig.h
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS))
// = 5 << 4 = 0x50 → 预期 BASEPRI = 0x50

// portmacro.h
#define portDISABLE_INTERRUPTS()   vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()    vPortSetBASEPRI(0)
#define portENTER_CRITICAL()       vPortEnterCritical()
#define portEXIT_CRITICAL()        vPortExitCritical()
```

`vPortEnterCritical` → `portDISABLE_INTERRUPTS()` → 写 BASEPRI
`vPortExitCritical` → 如果 `uxCriticalNesting == 0` 则恢复 BASEPRI 为 0

### 第六步：定位根本原因

```c
// port.c
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;  // ← 哨兵值
```

`sdlf_port_init()` 调用 `xSemaphoreCreateMutexStatic()`：
1. `taskENTER_CRITICAL()` → `vPortEnterCritical()`
   - `portDISABLE_INTERRUPTS()` → BASEPRI = 0x50
   - `uxCriticalNesting++` → 0xaaaaaaab
2. `taskEXIT_CRITICAL()` → `vPortExitCritical()`
   - `uxCriticalNesting--` → 0xaaaaaaaa
   - `0xaaaaaaaa == 0`? **否** → **BASEPRI 永不恢复**

此后所有优先级 ≥ 5 的中断（包括 TIM7@15）全部被永久屏蔽。

## FreeRTOS 为什么这样设计

### `uxCriticalNesting = 0xaaaaaaaa` 的用途

这是 FreeRTOS 的一个**防御性哨兵值**：

- 正常运行时（调度器已启动），`xPortStartScheduler()` 将 `uxCriticalNesting` 初始化为 0
- 如果应用程序在调度器启动**之前**使用了临界区 API（`taskENTER_CRITICAL` / `taskEXIT_CRITICAL`），由于初始值不是 0，退出临界区时不会被误判为"已经完全退出"
- 这实际上是一个**部分实现的保护机制**：它能防止临界区嵌套计数错乱导致过早恢复 BASEPRI，但副作用是让 BASEPRI 永久不恢复

### FreeRTOS 的设计假设

FreeRTOS 的设计假设是：**调度器启动后**才会有任务和 ISR 使用临界区。在调度器启动之前，不应该调用需要临界区保护的 FreeRTOS API（如 `xSemaphoreCreateMutexStatic`）。

但实际上，`xSemaphoreCreateMutexStatic` 在初始化静态互斥锁时也需要临界区保护（防止中断并发修改队列结构），这就造成了矛盾。

### 为什么 BASEPRI 恰好是 0x80 而不是 0x50

读取到的 BASEPRI = 0x80 (优先级 8)，而 `configMAX_SYSCALL_INTERRUPT_PRIORITY = 0x50` (优先级 5)。推测原因：

1. FreeRTOS 有两个 API 使用临界区：`xSemaphoreCreateMutexStatic()` 被调用了两次（tlsf_port_init + sd_init）
2. 某些 FreeRTOS 内部路径可能使用 `portSET_INTERRUPT_MASK_FROM_ISR()` 而非 `portENTER_CRITICAL()`
3. 编译器优化可能导致 BASEPRI 寄存器读取时机与写入不同步

无论如何，0x80 或 0x50 都会屏蔽 TIM7 (优先级 15)，所以精确值不影响结论。

## 推荐解决方案

### 思路：将 RTOS 资源初始化推迟到调度器启动之后

当前的问题根源是：在调度器启动前调用了 FreeRTOS API（`xSemaphoreCreateMutexStatic`），触发了临界区但无法正确恢复。

最优策略：
1. **硬件资源初始化**（tlsf_port_init、sd_init 的硬件部分）保留在主线程，调度器启动前完成
2. **RTOS 资源初始化**（互斥锁创建）推迟到调度器启动后，在 `init_task` 中调用
3. `TLSF_LOCK()` / `SD_LOCK()` 宏已经检查了 `xTaskGetSchedulerState() == taskSCHEDULER_RUNNING`，因此在互斥锁创建之前不会真正加锁

### 代码改造

#### 1. `tlsf_port.c` — 拆分初始化

```c
// 移除 xSemaphoreCreateMutexStatic 调用
void tlsf_port_init(void) {
    // 仅初始化内存池，不创建互斥锁
    // ... TLSF pool init ...
}

// 新增：在调度器启动后调用
void tlsf_port_rtos_init(void) {
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buffer);
#endif
}
```

#### 2. `sd.c` — 同理拆分

```c
int sd_init(void) {
    // 仅初始化 SD 卡硬件，不创建互斥锁
    // ... SD card init ...
}

void sd_rtos_init(void) {
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buffer);
#endif
}
```

#### 3. `main.c` — 在 init_task 中调用

```c
static void init_task(void *pvParameters)
{
    (void)pvParameters;

    /* RTOS 资源初始化 — 此时调度器已运行，临界区正常工作 */
    tlsf_port_rtos_init();
    sd_rtos_init();

    xTaskCreateStatic(led_task, "led", 128, NULL, 1,
                       led_task_stack, &led_task_tcb);
    xTaskCreateStatic(usb_task, "usb", 512, NULL, 2,
                       usb_task_stack, &usb_task_tcb);
    // ...
}
```

### 为什么这样更好

1. **`TICK_INT_PRIORITY` 保持 15** — CubeMX 默认生成值，不需要手动修改
2. **临界区语义正确** — 调度器启动后 `uxCriticalNesting=0`，进入/退出临界区正常工作
3. **`g_mutex_buffer` 天然零初始化** — `static StaticSemaphore_t g_mutex_buffer;` 由 C 运行时零初始化，无需额外处理
4. **`TLSF_LOCK()` 已有保护** — 检查 `xTaskGetSchedulerState() == taskSCHEDULER_RUNNING`，互斥锁创建前不会加锁
5. **不需要修改 FreeRTOS 或 HAL 源码** — 完全在 BSP 层解决问题

### 备选方案（不推荐）

- **方案 B**：在 `main()` 函数开头手动将 `uxCriticalNesting` 改为 0。不推荐：uxCriticalNesting 是 `static` 变量，无法从外部访问，且破坏 FreeRTOS 设计意图
- **方案 C**：将 `TICK_INT_PRIORITY` 改为小于 5 的值。不推荐：CubeMX 每次重新生成会覆盖，且将 HAL 时基设为极高优先级不符合惯例
