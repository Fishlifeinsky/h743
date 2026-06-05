# USB_MSC 模块

USB Mass Storage Class 模块，将 SD 卡映射为 USB 大容量存储设备（U 盘）。

## 架构

```
主机 (PC)  ←→  USB MSC Protocol  ←→  TinyUSB MSC Driver
                                         ↓
                              tud_msc_read10_cb / write10_cb  (usb_msc.c)
                                         ↓
                              HAL_SD_ReadBlocks / WriteBlocks  (HAL 库)
                                         ↓
                              hsd1 (SDMMC1)  ←→  SD 卡
```

## 回调映射

| TinyUSB MSC 回调 | 功能 |
|---|---|
| `tud_msc_capacity_cb` | 返回 SD 卡 BlockNbr / BlockSize |
| `tud_msc_inquiry_cb` | 返回厂商/产品字符串 |
| `tud_msc_test_unit_ready_cb` | 检测 SD 卡是否就绪 |
| `tud_msc_start_stop_cb` | 弹出/加载介质 |
| `tud_msc_read10_cb` | 读取扇区 → `HAL_SD_ReadBlocks` |
| `tud_msc_write10_cb` | 写入扇区 → `HAL_SD_WriteBlocks` |
| `tud_msc_is_writable_cb` | 返回写保护状态 |

## API

```c
#include "usb_msc.h"

void usb_msc_init(void);    // 缓存 SD 卡信息 (在 sd_init 之后调用)
```

## 用法

```c
// main.c 初始化顺序
sd_init();
usb_msc_init();       // 必须在 tusb_port_init 之前
tusb_port_init();     // 开始 USB 枚举
vendor_init();

while (1) {
    tusb_port_task();
    vendor_poll();
}
```

## 依赖

- `sd` — SD 卡模块 (`hsd1` 句柄, `sd_init`)
- `tinyusb_port` — TinyUSB 移植 (MSC Class Driver)
- STM32H7 HAL SD 驱动
