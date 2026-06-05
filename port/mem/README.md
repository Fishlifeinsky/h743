# STM32H743 内存布局

## 总览

| 区域 | 起始地址 | 大小 | 用途 | 跟踪剩余 |
|------|----------|------|------|----------|
| ITCM | 0x00000000 | 64KB | 代码执行，FLASH 存放，`mem_itcm_load()` 搬运 | 否 |
| DTCM | 0x20000000 | 128KB | 数据 TCM，CPU 独占，不经 Cache | 否 |
| RAM_D1 | 0x24000000 | 512KB | .data / .bss / heap / stack / 剩余 | 是 |
| RAM_D2 DMA | 0x30000000 | 256KB | SRAM1+SRAM2，DMA 缓冲区 | 是 |
| RAM_D2 USB | 0x30040000 | 32KB | SRAM3，USB/ETH 缓冲区 | 否 |
| RAM_D3 | 0x38000000 | 64KB | 低功耗保留，BDMA 可访问 | 否 |
| BKPRAM | 0x38800000 | 4KB | VBAT 供电保持 | 否 |
| SDRAM | 0xC0000000 | 32MB | 外部 SDRAM（TLSF 内存池） | 由 TLSF 管理 |

## 链接脚本段

| 段名 | 内存 | 加载 | 宏 |
|------|------|------|-----|
| `.itcm_ram_func` | ITCMRAM | FLASH | `MEM_ITCM_SECTION` |
| `.sram_dtcm` | DTCMRAM | NOLOAD | `MEM_SRAM_DTCM_SECTION` |
| `.sram_d1` | RAM_D1 | NOLOAD | `MEM_SRAM_D1_SECTION` |
| `.sram_d2_dma` | RAM_D2 | NOLOAD | `MEM_SRAM_D2_DMA_SECTION` |
| `.sram_d2_usb` | RAM_D2 | NOLOAD | `MEM_SRAM_D2_USB_SECTION` |
| `.sram_d3` | RAM_D3 | NOLOAD | `MEM_SRAM_D3_SECTION` |
| `.sram_bkp` | BKPRAM | NOLOAD | `MEM_SRAM_BKP_SECTION` |

## 使用示例

```c
#include "mem.h"

// 将变量放入指定内存区域
MEM_SRAM_D2_DMA_SECTION uint8_t dma_buf[4096];
MEM_SRAM_DTCM_SECTION   int   fast_data;
MEM_ITCM_SECTION        void  critical_func(void) { ... }

// 启动时搬运 ITCM 代码（在所有段初始化之后调用）
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    // ...
    mem_itcm_load();  // 搬运 .itcm_ram_func 段到 ITCM RAM
    // ...
}

// 查询空闲内存
uint32_t d1_free = MEM_SRAM_D1_FREE_SIZE;   // D1 剩余
uint32_t d2_dma_free = MEM_SRAM_D2_DMA_FREE_SIZE;  // D2 DMA 剩余
```

## ITCM 说明

- 代码存储在 FLASH（`.itcm_ram_func` 段 LMA）
- 链接时映射到 ITCM RAM（VMA = 0x00000000）
- 启动后需调用 `mem_itcm_load()` 从 FLASH 搬运到 ITCM
- ITCM 特性：0 wait，不经 Cache，仅 CPU 可访问

## 适配点

1. **链接脚本**: `STM32H743IITX_FLASH.ld` 定义所有内存区和段
2. **头文件**: `BSP/mem/include/mem.h` 提供函数声明、地址、大小、段属性宏（`MEM_` 前缀）
3. **搬运函数**: `BSP/mem/mem.c` 实现 `mem_itcm_load()`
4. **CubeMX 重置**: 修改 ld 或 CMakeLists 后需重新修复 `cmake_minimum_required` 和 BSP 相关行
