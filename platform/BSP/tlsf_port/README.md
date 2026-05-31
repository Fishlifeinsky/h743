# TLSF 内存分配器移植

TLSF (Two-Level Segregated Fit) 的 STM32H7 移植，支持双内存池（SDRAM + SRAM D1），FreeRTOS 互斥锁保护。

## 概述

| 内存池 | 地址 | 大小 | 用途 |
|---|---|---|---|
| `TLSF_PORT_SDRAM` | 0xC0000000 | 32MB | 大容量、慢速（外部 SDRAM） |
| `TLSF_PORT_SRAM_D1` | 0x24000000 | ~500KB | 小容量、高速（内部 SRAM D1） |

## API

```c
void  tlsf_port_init(void);      // 初始化内存池（调度器启动前调用）
void  tlsf_port_rtos_init(void); // 创建互斥锁（调度器启动后调用）
void *tlsf_port_malloc(size_t size, tlsf_port_pool_t pool);
void *tlsf_port_realloc(void *ptr, size_t size);
void  tlsf_port_free(void *ptr);
tlsf_t tlsf_port_get(tlsf_port_pool_t pool);
```

## 调用顺序

```c
// main() 中（调度器启动前）
tlsf_port_init();        // 仅创建 TLSF 内存池，不创建互斥锁

// init_task 中（调度器启动后）
tlsf_port_rtos_init();   // 创建互斥锁
```

## 锁机制

```c
#define TLSF_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
```

- `g_mutex` 为 `static` 变量，天然零初始化为 `NULL`
- 调度器启动前：锁宏为空操作（此时没有任务，无需保护）
- `tlsf_port_rtos_init()` 之后：互斥锁生效，所有 `malloc`/`free`/`realloc` 串行化

## 自动池识别

`tlsf_port_free()` 和 `tlsf_port_realloc()` 根据指针地址自动判断所属内存池：

```c
void *p = tlsf_port_malloc(1024, TLSF_PORT_SDRAM);
// ...
tlsf_port_free(p);  // 自动识别为 SDRAM 池
```

## 注意事项

- `tlsf_port_init()` 与 `tlsf_port_rtos_init()` 必须分开调用
- 调度器启动前不可调用 `xSemaphoreCreateMutexStatic` 等 FreeRTOS API（会导致 `BASEPRI` 永久置位）
