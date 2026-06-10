# port 模块规范

板级支持包 (port)，存放所有非 CubeMX 生成的外设驱动和第三方库移植代码。

## 目录结构

```
port/
├── <module_name>/                  # 模块目录，目录名 = 模块名
│   ├── include/
│   │   └── <module_name>.h         # 头文件（必须存在）
│   ├── <module_name>.c             # 模块主源文件
│   ├── <module_name>_<feature>.c   # 功能源文件（可选，如 _test.c）
│   └── module.cmake                # 模块构建描述（必须存在）
│
├── <libname>_port/                 # 第三方库移植模块
│   ├── include/
│   │   └── <libname>_port.h
│   ├── <libname>_port.c
│   └── module.cmake
│
├── config.h                        # 公共宏存放位置
└── MODULE_SPEC.md                  # 本文件
```

## 构建系统

每个模块通过 `module.cmake` 声明源文件和头文件路径，根目录 `CMakeLists.txt` 自动扫描 `port/*/module.cmake`，无需手动添加模块。

```cmake
# port/sd/module.cmake 示例
set(MODULE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/sd.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_test.c
)
set(MODULE_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include
)
```

`${CMAKE_CURRENT_LIST_DIR}` 自动指向 `module.cmake` 所在目录，适用于 `port/*/<module>.c` 和 `port/*/include` 路径模式。

## 文件命名

| 用途 | 格式 | 示例 |
|------|------|------|
| 头文件 | `<module_name>.h` | `sd.h` |
| 主源文件 | `<module_name>.c` | `sd.c` |
| 功能源文件 | `<module_name>_<feature>.c` | `sd_test.c` |
| 构建描述 | `module.cmake` | `module.cmake` |
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
// port/sd/include/sd.h
#define SD_READ_TIMEOUT  (30 * 1000)

// port/qspi_w25q64/include/qspi_w25q64.h
#define QSPI_FLASH_SIZE  (8 * 1024 * 1024)
```

不属于任何模块的公共开关（系统调用开关、单元测试开关、调试打印等）统一放在 `port/config.h` 中。

## 模块描述符

`port/config.h` 提供一套模块描述符宏，用于声明模块、管理依赖关系和初始化状态。

### 数据结构

```c
typedef struct module {
    const char*    name;     // 模块名
    bool           isInit;   // 是否已初始化
    struct module* depend;   // 依赖模块指针, 无依赖填 NULL
} module;
```

### 宏速查

| 宏 | 用途 |
|----|------|
| `MODULE_DECLARE(name, depend)` | 声明模块实例（放在模块 .c 顶部） |
| `MODULE_EXTERN(name)` | 外部引用其他模块 |
| `MODULE_IS_INIT(name)` | 检查模块是否已初始化 |
| `MODULE_INIT_DONE(name)` | 标记初始化完成，并打印 `[name] init done` |
| `MODULE_DEPEND_IS_INIT(name)` | 检查依赖是否已初始化 |
| `MODULE_DEPEND_IS_INIT_ASSERT(name)` | 断言依赖已初始化（debug 模式下打印诊断） |

### 示例

```c
// port/usb_msc/usb_msc.c

#include "usb_msc.h"

MODULE_EXTERN(tusb_port);                        // 引用依赖模块
MODULE_DECLARE(usb_msc, &mod_tusb_port);         // 声明本模块, 依赖 tusb_port

int usb_msc_init(void) {
    MODULE_DEPEND_IS_INIT_ASSERT(usb_msc);       // debug 模式下打印诊断
    if (!MODULE_DEPEND_IS_INIT(usb_msc)) return -1;  // 依赖未初始化则失败
    if (MODULE_IS_INIT(usb_msc)) return 0;       // 已初始化, 幂等

    // ... 硬件初始化 ...

    MODULE_INIT_DONE(usb_msc);                   // 标记完成 + 打印
    return 0;
}
```

无依赖模块：`MODULE_DECLARE` 第二参数传 `NULL`，`MODULE_DEPEND_IS_INIT` 始终返回 true。

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
- `module.cmake` 不应将其 `include` 目录加入到顶层 CMake 的 include 路径中

```
port/mem/
├── include/
│   ├── mem.h            # 对外公共头文件
│   └── mem_private.h    # 模块私有头文件（仅供 mem.c 包含）
├── mem.c
├── mem_test.c
└── module.cmake
```

## 移植配置头文件

提供给第三方库移植模块使用的配置宏，单独放在以该库命名的配置头文件中，**不混入模块主头文件**。

```
port/tinyusb_port/
├── include/
│   ├── tusb_port.h       # 移植模块公共接口
│   └── tusb_config.h     # TinyUSB 配置宏（供 tusb_port.c / 3rd/TinyUSB 使用）
├── tusb_port.c
└── module.cmake
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

- **禁止直接使用 `printf`**，必须使用 `port/config.h` 中的 `DEBUG_PRINT` 宏
- `DEBUG_PRINT` 由 `DEBUG_MESSAGE` 开关统一控制，关闭时编译为空，不占用代码空间

```c
// ✓ 正确
DEBUG_PRINT("addr: %p, size: %lu\r\n", ptr, size);
DEBUG_PRINT("write: sector=%lu, count=%lu\r\n", sector, count);

// ✗ 错误
printf("SD: init ok\r\n");
```

## 第三方库移植

模块名格式：`<库名>_port`，例如 `tinyusb_port/`、`tlsf_port/`、`fatfs_port/`、`freertos_port/`、`lvgl_port/`。
