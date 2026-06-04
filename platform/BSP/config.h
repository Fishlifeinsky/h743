#ifndef BSP_CONFIG_H_
#define BSP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// BSP 系统调用开关
//   1 — 使用 BSP/system 提供的 syscall 实现（__io_putchar 输出到 USART1）
//   0 — 回退到 Core/Src/syscalls.c 的默认桩
//--------------------------------------------------------------------+
#define BSP_SYSCALLS_ENABLED 1

//--------------------------------------------------------------------+
// FreeRTOS 开关
//   1 — 启用 FreeRTOS 实时操作系统
//   0 — 裸机运行
//--------------------------------------------------------------------+
#define BSP_FREERTOS_ENABLED 0

//--------------------------------------------------------------------+
// BSP 单元测试开关
//   1 — 编译所有 BSP 模块的单元测试代码
//   0 — 不编译单元测试
//--------------------------------------------------------------------+
#define USE_BSP_UNIT_TESTING 1

#define DEBUG_UART huart1

//--------------------------------------------------------------------+
// 已使用的串口 (用于 uart 模块初始化数组)
//   格式: &huart1, &huart2, ...
//--------------------------------------------------------------------+
#define USED_UARTS  &huart1

//--------------------------------------------------------------------+
// 默认 malloc 内存池
//   TLSF_PORT_SRAM_D1 — 内部 SRAM D1 (~500KB free), 速度快
//   TLSF_PORT_SDRAM   — 外部 SDRAM (32MB), 容量大，速度较慢
//--------------------------------------------------------------------+
#define BSP_MALLOC_DEFAULT_POOL TLSF_PORT_SRAM_D1

//--------------------------------------------------------------------+
// 调试打印开关
//   1 — 开启 DEBUG_PRINT 宏，输出调试信息到串口
//   0 — 关闭，DEBUG_PRINT 编译为空
//--------------------------------------------------------------------+
#define DEBUG_MESSAGE 1

#if DEBUG_MESSAGE
#include "usart.h"
#define DEBUG_PRINT(fmt, ...) debug_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
