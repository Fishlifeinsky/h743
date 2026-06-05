# BSP 模块规范

板级支持包，存放所有非 CubeMX 生成的外设驱动和第三方库移植代码。

## 目录结构

```
BSP/
├── <module_name>/                  # 模块目录，目录名 = 模块名
│   ├── include/
│   │   └── <module_name>.h         # 头文件（必须存在）
│   ├── <module_name>.c             # 模块主源文件
│   └── <module_name>_<feature>.c   # 功能源文件（可选，如 _test.c）
│
├── <libname>_port/                 # 第三方库移植模块
│   ├── include/
│   │   └── <libname>_port.h
│   └── <libname>_port.c
│
├── config.h                        # 公共宏存放位置
├── CMakeLists.txt                  # 自动扫描所有模块，无需手动添加
└── MODULE_SPEC.md                  # 本文件
```

## 文件命名

| 用途 | 格式 | 示例 |
|------|------|------|
| 头文件 | `<module_name>.h` | `sd.h` |
| 主源文件 | `<module_name>.c` | `sd.c` |
| 功能源文件 | `<module_name>_<feature>.c` | `sd_test.c` |
| 第三方库移植 | `<libname>_port.h` / `<libname>_port.c` | `tlsf_port.h` |

## 函数命名

外部可调用函数：`<模块名>_xx`

```c
// ✓ 正确
int sd_init(void);
int sd_read(uint8_t *buf, uint32_t block, uint32_t count);
void qspi_w25q64_init(void);
```

内部函数使用 `static` 限定作用域。

## 宏命名

供外部使用的宏：`<模块名大写>_xx`，**定义在模块自己的头文件中**。

```c
// sd/include/sd.h
#define SD_READ_TIMEOUT  (30 * 1000)

// qspi/include/qspi_w25q64.h
#define QSPI_FLASH_SIZE  (8 * 1024 * 1024)
```

不属于任何模块的公共开关（系统调用开关、单元测试开关、调试打印等）统一放在 `BSP/config.h` 中。

## 注释规则

- 必须使用**中文注释**
- `static` 静态函数、可外部调用函数、内部全局变量之间必须用注释分隔

```c
//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

int sd_init(void) {
    ...
}

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static void foo(void) {
    ...
}

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static uint32_t g_buf[256];
```

## 模块私有头文件

模块可创建 `<module_name>_private.h` 存放仅供模块内部使用的声明（内部函数、内部宏、不对外暴露的数据结构）。

- 该文件由模块 `.c` 文件包含，**不对外暴露**
- `BSP/CMakeLists.txt` 不应将其加入到顶层 CMake 的 include 路径中

```
BSP/mem/
├── include/
│   ├── mem.h            # 对外公共头文件
│   └── mem_private.h    # 模块私有头文件（仅供 mem.c 包含）
├── mem.c
└── mem_test.c
```

## 移植配置头文件

提供给第三方库移植模块使用的配置宏，单独放在以该库命名的配置头文件中，**不混入模块主头文件**。

```
BSP/tinyusb_port/
├── include/
│   ├── tusb_port.h       # 移植模块公共接口
│   └── tusb_config.h     # TinyUSB 配置宏（供 tusb_port.c / 3rd/TinyUSB 使用）
├── tusb_port.c
```

```c
// ✓ 正确: tusb_config.h 独立存放
#include "tusb_config.h"

// ✗ 错误: 不要把 tusb_config 宏塞进 tusb_port.h
```

## 固定名称函数

回调函数、系统调用桩、标准库接口函数等有固定命名的函数，**保持原名不变**，不使用模块前缀。

```c
// 固定名称 — 保持原名
int __io_putchar(int ch);           // newlib 字符输出
int _write(int file, char *p, int n); // newlib 系统调用
void HAL_QSPI_RxCpltCallback(...);   // HAL 中断回调
void tud_vendor_rx_cb(uint8_t itf);  // TinyUSB 回调
```

## 打印输出

- **禁止直接使用 `printf`**，必须使用 `BSP/config.h` 中的 `DEBUG_PRINT` 宏
- `DEBUG_PRINT` 由 `DEBUG_MESSAGE` 开关统一控制，关闭时编译为空，不占用代码空间

```c
// ✓ 正确
DEBUG_PRINT("SD: init ok\r\n");
DEBUG_PRINT("addr: %p, size: %lu\r\n", ptr, size);

// ✗ 错误
printf("SD: init ok\r\n");
```

## 第三方库移植

模块名格式：`<库名>_port`，例如 `tinyusb_port/`、`tlsf_port/`。
