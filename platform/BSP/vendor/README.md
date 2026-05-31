# VENDOR 模块

基于 TinyUSB Vendor Class 的 USB 数据传输与 Flash 编程协议。

## 命令列表

| 命令 | 参数 | 说明 |
|------|------|------|
| `VB+BUFF` | `[size]` | 查询/设置 RX 缓冲区大小 |
| `VB+FLASH` | 无 | 查询外部 Flash 信息 (容量, MID, DID) |
| `VB+LOAD` | `addr,size` | 擦除目标区域，进入下载就绪状态 |
| `VB+DATA` | `hexdata` | 下载 hex 编码的二进制数据 |
| `VB+ABORT` | 无 | 中止当前传输，恢复内存映射模式 |

## 协议格式

```
请求:  VB+<命令>[=<参数>]\r\n
响应:  VB+<命令>=<结果>\r\n
```

## 状态机

```
IDLE --VB+LOAD(addr,size)--> LOADED --VB+DATA(hex)--> LOADED
  ^                            |                         |
  |---- VB+ABORT --------------|-------------------------|
```

## 下载流程

1. `VB+BUFF=<size>` 设置 RX 缓冲区大小 (可选，默认 512)
2. `VB+LOAD=<addr>,<size>` 擦除目标区域，进入下载就绪状态
3. 多次 `VB+DATA=<hex>` 发送 hex 编码的二进制数据
4. `VB+ABORT` 结束传输 (或自动校验完成)

## Flash 地址映射

| 区域 | 起始地址 | 大小 | 扇区 |
|------|----------|------|------|
| 内部 Flash | `0x08000000` | 2 MB | 128 KB × 8 |
| 外部 Flash (QSPI W25Q64) | `0x90000000` | 8 MB | 4 KB / 32 KB / 64 KB |

## API

```c
#include "vendor.h"

void vendor_init(void);   // 初始化 VENDOR 模块 (在 tusb_port_init 之后调用)
void vendor_poll(void);   // VENDOR 协议轮询 (需在主循环中调用)
```

```c
#include "vendor_flash.h"

// 获取外部 Flash 信息 (JEDEC ID)
void vendor_flash_get_info(vendor_flash_info_t *info);

// 擦除 / 写入 (按地址自动分发内部/外部 Flash)
int vendor_flash_erase(uint32_t addr, uint32_t size);
int vendor_flash_write(uint32_t addr, const uint8_t *data, uint32_t size);

// 直接操作内部 Flash
int vendor_flash_erase_internal(uint32_t addr, uint32_t size);
int vendor_flash_write_internal(uint32_t addr, const uint8_t *data, uint32_t size);

// 直接操作外部 Flash (W25Q64)
int vendor_flash_erase_external(uint32_t addr, uint32_t size);
int vendor_flash_write_external(uint32_t addr, const uint8_t *data, uint32_t size);
```

## 用法

```c
// 初始化
tusb_port_init();
vendor_init();

// 主循环
while (1) {
    tusb_port_task();
    vendor_poll();
}
```

## 依赖

- `tusb_port` — TinyUSB 移植模块 (USB Vendor Class)
- `qspi_w25q64` — W25Q64 外部 Flash 驱动
- `mem` — TLSF 内存管理 (ITCM hex 解码函数)
- STM32H7 HAL Flash / QSPI 驱动
