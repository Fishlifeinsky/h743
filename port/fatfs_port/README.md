# FATFS_PORT 模块

FatFs 文件系统移植模块，提供 SD 卡底层操作接口给 `3rd/fatfs/diskio.c` 调用。

## 文件结构

```
BSP/fatfs_port/
├── include/
│   ├── fatfs_port.h       # 移植模块公共接口
│   └── ffconf.h           # FatFs 配置 (从此模块为准)
├── fatfs_port.c           # 硬件实现 (基于 HAL SD)
└── README.md

3rd/fatfs/
├── diskio.c               # FatFs 胶水层: disk_* → fatfs_port_sd_*
├── diskio.h               # FatFs 磁盘 I/O 接口定义
├── ff.c / ff.h            # FatFs 核心
├── ffsystem.c             # 系统接口
├── ffunicode.c            # Unicode 支持
└── CMakeLists.txt
```

## 架构

```
f_mount / f_read / f_write   ← 用户调用 (FatFs API)
        ↓
disk_read / disk_write 等     ← 3rd/fatfs/diskio.c (胶水层, 固定名称)
        ↓
fatfs_port_sd_read / write   ← BSP/fatfs_port/fatfs_port.c (硬件实现)
        ↓
HAL_SD_ReadBlocks / WriteBlocks  ← HAL 库
        ↓
hsd1 (SDMMC1)                ← CubeMX 生成
```

## API

```c
#include "fatfs_port.h"

// FatFs 挂载
int  fatfs_port_init(void);       // f_mount 挂载 SD 卡文件系统
void fatfs_port_get_info(void);   // 打印 SD 卡信息

// SD 卡底层操作 (供 3rd/fatfs/diskio.c 调用)
int fatfs_port_sd_init(void);
int fatfs_port_sd_status(void);
int fatfs_port_sd_read(uint8_t *buf, uint32_t sector, uint32_t count);
int fatfs_port_sd_write(const uint8_t *buf, uint32_t sector, uint32_t count);
int fatfs_port_sd_ioctl(uint8_t cmd, void *buf);
```

## 用法

```c
// 初始化 (sd_init 由 disk_initialize → fatfs_port_sd_init 自动触发)
fatfs_port_init();

// 文件操作
FIL fil;
if (f_open(&fil, "test.txt", FA_READ) == FR_OK) {
    // ...
    f_close(&fil);
}
```

## 依赖

- `sd` — SD 卡初始化模块 (`sd_init`, `hsd1` 句柄)
- `3rd/fatfs` — FatFs 核心库
- STM32H7 HAL SD 驱动
