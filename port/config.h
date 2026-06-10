#ifndef BSP_CONFIG_H_
#define BSP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

//--------------------------------------------------------------------+
// 通用模块描述符
//--------------------------------------------------------------------+

typedef struct module{
    const char* name;       // 模块名
    bool isInit;            // 是否已初始化
    struct module* depend;     // 依赖模块名, 无依赖填 NULL
}module;

// 生成模块变量名: MODULE_NAME(usb_vdb) → mod_usb_vdb
#define MODULE_NAME(_name)            mod_##_name

// 声明模块实例 (放在模块 .c 文件顶部)
//   用法: MODULE_DECLARE(usb_vdb, &mod_tusb_port);
//   无依赖: MODULE_DECLARE(usb_vdb, NULL);
#define MODULE_DECLARE(_name, dename)    module mod_##_name = { .name = #_name, .isInit = false, .depend = dename }

// 外部引用其他模块 (放在头文件或调用方 .c 顶部)
#define MODULE_EXTERN(_name)          extern module mod_##_name

// 检查模块是否已初始化
#define MODULE_IS_INIT(_name)         (mod_##_name.isInit)

// 标记模块初始化完成
#define MODULE_INIT_DONE(_name)       do { (mod_##_name.isInit = true); DEBUG_PRINT("[%s] init done\r\n", mod_##_name.name); } while(0)

// 判断依赖是否初始
#define MODULE_DEPEND_IS_INIT(_name)  (!mod_##_name.depend || mod_##_name.depend->isInit)

#define MODULE_DEPEND_IS_INIT_ASSERT(_name) \
DEBUG_ASSERT(MODULE_DEPEND_IS_INIT(_name),"[%s] depend[%s] is not init\n", MODULE_NAME(_name).name, MODULE_NAME(_name).depend?MODULE_NAME(_name).depend->name:"NULL")
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
#define BSP_FREERTOS_ENABLED 1

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
#define DEBUG_ASSERT(_code,fmt,...) if(!_code) {debug_printf(fmt, ##__VA_ARGS__);while(0){}}
#else
#define DEBUG_PRINT(fmt, ...)  ((void)0)
#define DEBUG_ASSERT(_code, fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
