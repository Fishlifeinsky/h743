# Platform

STM32H743 平台库，聚合所有 3rd 三方库 + BSP 板级支持包 + LIB 通用库，编译为单个 `libplatform.a` 静态库。

## 目录结构

```
platform/
├── 3rd/                    第三方库
│   ├── TinyUSB/            USB 设备栈 (VENDOR + MSC, DWC2 OTG FS)
│   ├── tlsf/               TLSF 实时内存分配器 (v3.1)
│   └── fatfs/              FatFs FAT 文件系统 (R0.16)
│
├── BSP/                    板级支持包 (Board Support Package)
│   ├── config.h            全局开关 (单元测试 / 调试打印 / 系统调用)
│   ├── system/             newlib syscall 实现 + UART 串口 I/O
│   ├── mem/                内存布局宏 + ITCM / SDRAM 搬运
│   ├── sdram/              SDRAM 初始化时序
│   ├── sd/                 SD 卡初始化 (SDMMC1)
│   ├── qspi_w25q64/        W25Q64 QSPI NOR Flash 驱动
│   ├── tlsf_port/          TLSF 多池内存分配器移植
│   ├── fatfs_port/         FatFs SD 卡挂载 + VFS 桥接
│   ├── tinyusb_port/       TinyUSB 设备描述符 + 初始化
│   ├── usb_msc/            USB MSC 回调 (SD 卡模拟 U 盘)
│   └── vendor/             USB VENDOR 协议 (远程烧录 Flash)
│
└── LIB/                    通用库
    ├── elib/               嵌入式单向链表
    └── vfs/                虚拟文件系统 (挂载点 + 路径匹配)
```

## 架构

```
应用层 (newlib stdio: fopen / fread / fwrite / printf ...)
    │
    ▼
system.c (newlib syscall 桩: _open / _read / _write / _close ...)
    │
    ▼
VFS (虚拟文件系统 — 最长前缀匹配，分发到已注册节点)
    │
    ▼
fatfs_port (VFS 节点 "/sd"，封装 FatFs f_open / f_read / f_write ...)
    │
    ▼
FatFs (FAT 文件系统) → diskio.c (胶水层) → SDMMC1 HAL → SD 卡
```

关键分层：

| 层 | 职责 |
|---|---|
| **3rd/** | 原版三方代码，仅抑制警告，不做逻辑改动 |
| **BSP/** | 硬件相关移植/驱动，每个模块一个目录，自包含 |
| **LIB/** | 硬件无关的通用数据结构/协议库 |
| **VFS** | 连接 syscall 与具体文件系统的中间层，支持多节点挂载 |

## BSP 模块规范

每个 BSP 模块是一个独立目录，结构如下：

```
BSP/<module>/
├── include/            # 头文件 (对外 API)
│   └── <module>.h
├── <module>.c          # 主实现
├── <module>_test.c     # 单元测试 (受 USE_BSP_UNIT_TESTING 宏控制)
└── README.md           # (可选) 模块说明
```

- 新增模块只需创建目录和文件，CMake 自动收录
- 模块间通过 `include/` 暴露 API，通过 `config.h` 获取全局宏
- 系统调用统一由 `BSP/system/system.c` 处理，其他模块不直接实现 newlib 桩

## 全局配置 (`BSP/config.h`)

| 宏 | 作用 |
|---|---|
| `BSP_SYSCALLS_ENABLED` | 是否使用 BSP/system 的 syscall 实现 |
| `USE_BSP_UNIT_TESTING` | 是否编译各模块的单元测试 |
| `DEBUG_MESSAGE` | 是否启用 `DEBUG_PRINT` 串口调试输出 |

## 依赖注入

VFS 和 elib 通过 CMake 编译选项注入依赖头文件，不硬编码 include 路径：

```cmake
target_compile_options(platform PRIVATE
    "-DVFS_LIST_H=\"elib/list.h\""      # VFS 使用的链表库
    "-DVFS_MALLOC_H=\"tlsf_port.h\""    # VFS 使用的内存分配器
)
```

## 单元测试

`USE_BSP_UNIT_TESTING=1` 时各模块的 `*_test.c` 被编译，共有测试：

| 模块 | 测试文件 | 内容 |
|---|---|---|
| `tlsf_port` | `tlsf_port_test.c` | 多池分配/释放/重分配 |
| `fatfs_port` | `fatfs_port_test.c` | FatFs 原生 API 读写校验 + 目录遍历 |
| `fatfs_port` | `fatfs_port_vfs_test.c` | 通过标准 stdio 经 VFS 读写 SD 卡 |
| `sd` | `sd_test.c` | SD 擦除/写入/读回校验 |
| `qspi_w25q64` | `qspi_w25q64_test.c` | W25Q64 擦除/写入/读回校验 |
| `sdram` | `sdram_test.c` | SDRAM 读写速度测试 |

测试在 `main.c` 的初始化阶段顺序执行，输出结果到串口。
