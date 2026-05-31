# BSP 板级支持包

存放所有非 CubeMX 生成的外设驱动、第三方库移植代码和系统级组件。

## 目录结构

```
BSP/
├── config.h              # 公共开关（FreeRTOS、调试打印、已用串口等）
├── MODULE_SPEC.md        # 模块编码规范
├── fatfs_port/           # FatFS 移植（SD 卡文件系统）
├── freertos_port/        # FreeRTOS 配置与启动封装
├── lcd/                  # LCD 驱动（LTDC + DMA2D）
├── mem/                  # 内存布局与段加载
├── qspi_w25q64/          # W25Q64 QSPI Flash 驱动
├── sd/                   # SD 卡 HAL 封装
├── sdram/                # SDRAM 初始化序列
├── system/               # 系统调用与延时函数
├── tinyusb_port/         # TinyUSB 设备移植
├── tlsf_port/            # TLSF 内存分配器移植
├── uart/                 # 串口封装（LL RXNE + StreamBuffer）
├── usb_msc/              # USB Mass Storage 类
└── vendor/               # 自定义 VENDOR 协议（配合 TinyUSB）
```

## 初始化顺序

```c
// main() 中（调度器启动前）
HAL_Init();
SystemClock_Config();
MX_*_Init();         // CubeMX 生成的外设初始化
lcd_init();          // LTDC + DMA2D

// 硬件级初始化（不调用 FreeRTOS API）
qspi_w25q64_init();
qspi_w25q64_memory_mapped_mode();
sdram_init_sequence();
mem_itcm_load();
mem_sdram_load();
tlsf_port_init();    // 仅初始化内存池，不创建互斥锁
sd_init();           // 仅初始化 SD 卡硬件，不创建互斥锁
uart_init();         // 仅初始化硬件和 RXNE 中断，不创建 RTOS 资源
fatfs_port_init();
tusb_port_init();
// ...

// 调度器启动后（init_task 中）
tlsf_port_rtos_init();  // 创建互斥锁
sd_rtos_init();         // 创建互斥锁
uart_rtos_init();       // 创建互斥锁、流缓冲区、轮询任务
```

### 为什么要拆分 `*_init()` 和 `*_rtos_init()`

FreeRTOS 的 `uxCriticalNesting` 初始值为哨兵值 `0xaaaaaaaa`，在调度器启动前调用任何进入临界区的 FreeRTOS API（如 `xSemaphoreCreateMutexStatic`）会导致 `BASEPRI` 被永久置位，屏蔽所有低优先级中断。

**规则**：所有调用 FreeRTOS API 创建 RTOS 资源的操作，必须在 `vTaskStartScheduler()` 之后、在 `init_task` 中执行。

| 阶段 | 可调用 | 不可调用 |
|------|--------|----------|
| `main()` 中（调度器启动前） | HAL 函数、寄存器操作、纯数据结构初始化 | `xSemaphoreCreateMutexStatic`、`xTaskCreateStatic`、`xStreamBufferCreateStatic` 等 FreeRTOS API |
| `init_task` 中（调度器启动后） | 所有 API | — |

## FreeRTOS vs 裸机

由 `BSP/config.h` 中的 `BSP_FREERTOS_ENABLED` 统一控制：

- `1` — 编译 FreeRTOS 相关代码，启用多任务调度
- `0` — 裁剪 FreeRTOS 代码，裸机运行

所有涉及 FreeRTOS API 的代码都用 `#if BSP_FREERTOS_ENABLED` / `#endif` 包裹：

```c
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buffer);
#endif
```

## 锁宏模式（FreeRTOS 环境）

互斥锁的加锁/解锁统一使用 `if (g_mutex)` 判断，**不检查调度器状态**：

```c
#define MY_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
#define MY_UNLOCK() do { if (g_mutex) xSemaphoreGive(g_mutex); } while(0)
```

- `g_mutex` 是 `static` 变量，天然零初始化为 `NULL`
- 调度器启动前 `g_mutex == NULL`，锁宏为空操作
- `*_rtos_init()` 在 `init_task` 中创建互斥锁后，`g_mutex` 变为有效值

## 模块列表

| 模块 | 功能 | 头文件 |
|------|------|--------|
| `fatfs_port` | FatFS 移植到 SD 卡，VFS 注册 | `fatfs_port.h` |
| `freertos_port` | FreeRTOS 配置头、启动封装 | `freertos_port.h` |
| `lcd` | LTDC 显示 + DMA2D 加速 | `lcd.h` |
| `mem` | 内存段定义、代码/数据搬运到 ITCM/SDRAM | `mem.h` |
| `qspi_w25q64` | W25Q64 QSPI Flash 读写与内存映射 | `qspi_w25q64.h` |
| `sd` | SD 卡 HAL 封装、互斥锁保护读写 | `sd.h` |
| `sdram` | SDRAM 初始化命令序列 | `sdram.h` |
| `system` | 系统调用（newlib）、延时函数 | `system_delay.h` |
| `tinyusb_port` | TinyUSB 设备栈移植 | `tusb_port.h` |
| `tlsf_port` | TLSF 内存分配器（双池：SRAM D1 + SDRAM） | `tlsf_port.h` |
| `uart` | 串口 LL RXNE 中断 + FreeRTOS StreamBuffer | `uart.h` |
| `usb_msc` | USB Mass Storage 类（配合 TinyUSB） | `usb_msc.h` |
| `vendor` | 自定义 VENDOR 类协议 | `vendor.h` |

## 编码规范

见 [MODULE_SPEC.md](MODULE_SPEC.md)。

要点：
- 中文注释
- 外部函数使用 `<模块名>_xx` 前缀
- 禁止直接使用 `printf`，统一使用 `DEBUG_PRINT`
- 模块头文件放在 `include/` 子目录中
