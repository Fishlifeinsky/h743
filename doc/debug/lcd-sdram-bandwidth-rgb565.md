# LCD 画面鬼影/闪动排查

## 现象

用 LVGL 显示时，屏幕出现：
1. **对角线黑线波浪** — 侧看特定灰阶背景时可见（LCD 面板光学特性，与信号无关）
2. **控件跳动/位移** — 帧率监控每帧刷新 FPS 数字时，中间按钮/其他控件在同一水平线不同 X 位置反复出现

裸机直接用 CPU 写 SDRAM 帧缓冲复现：在右下角 `lcd_fill_rect(680,400,100,40)` 画黑块，5ms 循环刷新，结果**同 Y 坐标、前方 X 位置**出现黑色鬼影。

## 排除过程

| 测试 | 结论 |
|------|------|
| 关 FreeRTOS 裸机跑 | 无变化 — **排除 RTOS 问题** |
| 关 LV_USE_PERF_MONITOR | 跳动频率降低 — **排除 LVGL bug，指向刷新频率相关** |
| CPU 直接写全屏 `fb[i]=color` 循环 | 全屏纯色正常 — **排除 SDRAM 地址/Bank 硬件故障** |
| 关 D-Cache (`SCB_DisableDCache`) | 无变化 — **排除 D-Cache 一致性问题** |
| 关 I-Cache | 无变化 — **排除 I-Cache** |
| DMA2D flush 替代 CPU flush | 画面全黑 — **DMA2D 无法读 DTCM** |
| 透写模式 + 双缓冲 + vsync 交换 | 无变化 — **排除单缓冲撕裂** |
| 改 SDRAM WriteRecoveryTime/RPDelay | 无变化 — **排除 SDRAM 写入时序** |
| **改 LTDC 层 RGB565 (2B/pixel)** | **鬼影消失** |

## 根因

**SDRAM 带宽不足 → LTDC FIFO 下溢 → 像素读地址指针错位**

```
LTDC 33MHz 从 SDRAM 读 ARGB8888: 33M × 4B = 132 MB/s
CPU 同时写 SDRAM:                       ~3–5 MB/s
16-bit SDRAM @ 100MHz 实际效率 60%:    ~120 MB/s
                                    ───────────────
                                    合计 > 可用带宽

LTDC 内部 FIFO 仅 32 像素 (~970ns 缓冲)
CPU 紧循环写期间 LTDC 读请求被阻塞
→ 970ns 后 FIFO 空 → 读地址指针错位
→ 同列地址偏移 → 同一行不同位置出现鬼影
```

**为什么全屏写不出现：** 全屏循环每次覆写所有像素 → 即使个别写入时 FIFO 下溢导致错读，下一轮全覆盖抹去差异。

**为什么 RGB565 能解决：** RTDC 读带宽从 132MB/s → 66MB/s，SDRAM 负载从 ~140MB/s → ~70MB/s，回到安全区间。

## 相关更改

- `platform/BSP/lcd/lcd.c`: `LTDC_PIXEL_FORMAT_ARGB8888` → `RGB565`，帧缓冲 `*4` → `*2`
- `platform/BSP/lcd/include/lcd.h`: 颜色宏/函数签名 `uint32_t` → `uint16_t`
- `platform/BSP/lvgl_port/include/lv_conf.h`: `LV_COLOR_DEPTH 32` → `16`
- `platform/BSP/lvgl_port/lv_port.c`: `FB_SIZE *4` → `*2`

## 参考

ESP32 LCD RGB 面板类似问题：GDMA FIFO under-run → 读地址错位 → 画面漂移。
解决方向：bounce buffer / 降低像素时钟 / 缩小色深减少带宽。
