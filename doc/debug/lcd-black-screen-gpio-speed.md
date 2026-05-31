# LCD 黑屏：显存正常刷新但屏幕不点亮

## 版本信息

- 芯片: STM32H743IIT6 (Cortex-M7)
- LCD: 800×480 RGB 屏 (ARGB8888, 单层)
- LTDC 时钟: 33MHz (PLL3: M=25, N=330, R=10)
- 显存: 外部 SDRAM (0xC0000000)
- 现象触发条件: 移除了反客例程的 `lcd_rgb.c`，改用 CubeMX 生成的 `ltdc.c`

## 现象

LCD 屏幕完全不亮（黑屏），但串口调试输出显示所有 LCD 测试项正常执行：

```
[LCD] 清屏色测试
[LCD] 整帧刷新时间测试 (DWT, HCLK=240MHz)
  DMA2D fill 800x480: 49712 us (11930995 cyc)
  逐点填充 800x480: 62044 us (14890777 cyc)
[LCD] 文字测试
[LCD] 颜色测试
[LCD] RGB渐变测试
[LCD] 2D图形测试
[LCD] 测试完成
```

DMA2D 整帧填充耗时约 50ms，逐点填充约 62ms，证明显存读写完全正常。

## 调试过程

### 第一步：确认显存是否被正确写入

DMA2D 填充和逐点填充均正常完成，SDRAM 访问无异常。排除显存问题。

### 第二步：确认背光是否打开

示波器探测 PH6（背光控制引脚），确认为高电平。排除背光问题。

### 第三步：确认 LTDC 是否被正确初始化

分析调用链：

```
main.c → MX_LTDC_Init()   (ltdc.c, 函数体被 #if 0 包围，空函数)
      → lcd_init()         (lcd.c, 手动编写的 LTDC 初始化)
        → HAL_LTDC_Init()
          → HAL_LTDC_MspInit()  (ltdc.c, CubeMX 生成)
            → 配置 PLL3、GPIO、LTDC 时钟
```

`lcd_init()` 中 LTDC 时序参数（HSW=1, HBP=80, HFP=200, VSW=1, VBP=40, VFP=22）与反客参考例程一致。LTDC 配置正确。

### 第四步：对比反客参考例程

参考例程路径：`反客STM32H743IIT6核心板\LTDC液晶驱动\1.显示驱动\lcd_rgb.c`

逐项对比：

| 检查项 | 本工程 | 反客例程 | 状态 |
|---|---|---|---|
| LTDC 时序参数 | H=1,80,200 V=1,40,22 | H=1,80,200 V=1,40,22 | 一致 |
| PLL3 配置 | M25/N330/R10 → 33MHz | M25/N330/R10 → 33MHz | 一致 |
| LTDC 极性 | HS=AL, VS=AL, DE=AL, PC=IPC | HS=AL, VS=AL, DE=AL, PC=IPC | 一致 |
| 背光引脚 | PH6 | PH6 | 一致 |
| **LTDC GPIO 速度** | **`GPIO_SPEED_FREQ_LOW`** | `GPIO_SPEED_FREQ_VERY_HIGH` | **不一致** |
| 显存地址 | SDRAM 0xC0000000 | SDRAM 0xC0000000 | 一致 |

### 第五步：确认根因

STM32H743 GPIO 速度等级对应的最大输出频率（估算值）：

| 等级 | 典型上限 |
|---|---|
| `GPIO_SPEED_FREQ_LOW` | ~10 MHz |
| `GPIO_SPEED_FREQ_MEDIUM` | ~50 MHz |
| `GPIO_SPEED_FREQ_HIGH` | ~100 MHz |
| `GPIO_SPEED_FREQ_VERY_HIGH` | ~200 MHz |

LTDC 像素时钟 = 33MHz。26 根并行 RGB 数据线 + HSYNC + VSYNC + DE + CLK 共 27 根信号线**全部**配置为 `GPIO_SPEED_FREQ_LOW`（~10MHz 上限）。

**根本原因**：GPIO 驱动能力不足，33MHz 信号在 `GPIO_SPEED_FREQ_LOW` 模式下输出波形边沿严重退化。LCD 面板控制器无法正确锁存像素数据、同步信号和时钟，导致屏幕完全黑屏。

## 根因

CubeMX 在生成 `ltdc.c` 的 `HAL_LTDC_MspInit()` 时，LTDC 引脚默认使用 `GPIO_SPEED_FREQ_LOW`。ST 的默认值未考虑 LTDC 高速并行 RGB 的实际带宽需求。

反客例程在自己的 `HAL_LTDC_MspInit()` 中**显式**将所有 LTDC 引脚设为 `GPIO_SPEED_FREQ_VERY_HIGH`。

当本项目从反客例程迁移到 CubeMX 生成的 `ltdc.c` 后，GPIO 速度降级，屏幕不再点亮。

## 解决方案

将 `Core/Src/ltdc.c` 中 `HAL_LTDC_MspInit()` 内所有 LTDC 引脚的 `GPIO_InitStruct.Speed` 从 `GPIO_SPEED_FREQ_LOW` 改为 `GPIO_SPEED_FREQ_VERY_HIGH`。

涉及 8 个 GPIO 端口（PE, PI, PF, PA, PH, PG, PD）共 27 根信号线。

## 教训

- 高速并行总线（LTDC、FMC、QSPI 等）的 GPIO 速度等级必须匹配实际信号频率
- CubeMX 的默认 GPIO 速度设置（`LOW`）不可盲信，需根据外设时钟手动调整
- 排查 LCD 不亮问题时，排除显存和背光后，用示波器测 CLK/DATA 信号波形是快速定位 GPIO 驱动问题的手段
- 移植参考例程时，注意对比 HAL 弱回调（`HAL_*_MspInit`）中的硬件配置细节
