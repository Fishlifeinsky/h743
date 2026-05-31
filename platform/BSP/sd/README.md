# SD 卡模块

SDMMC1 接口的 SD 卡初始化和读写驱动，直接调用 HAL 库。

## API

```c
int  sd_init(void);      // 初始化 SD 卡硬件 (4-bit 宽总线 + 高速模式)
void sd_rtos_init(void); // 创建 RTOS 互斥锁 (调度器启动后调用)
void sd_test(void);      // 单元测试 (需定义 USE_BSP_UNIT_TESTING)
```

`sd_init()` 由 `main.c` 在 `MX_SDMMC1_SD_Init()` 之后调用，完成宽总线和高速模式配置，并打印卡信息和时钟频率。

`sd_rtos_init()` 在调度器启动后的 `init_task` 中调用，创建互斥锁。不可在调度器启动前调用。

`sd_test()` 执行以下测试：
1. 擦除 (block 0~63, 共 32KB)
2. 写入 (32KB 递增模式)
3. 单次读取 + 校验
4. 连续读 100 轮 (带超时保护和错误恢复)

## 时钟配置

```
SDMMC_CK = SDMMCCLK / (2 × ClockDiv)
```

`SDMMCCLK` 由 `RCC_SDMMCCLKSOURCE_PLL` 选择 PLL1Q 或 PLL2R 作为时钟源。
`ClockDiv` 在 `sdmmc.c` 的 `MX_SDMMC1_SD_Init()` 中设置。

| ClockDiv | 分频比 | 说明 |
|----------|--------|------|
| 0 | /1 | 最大速度，不分频 |
| 1 | /2 | 推荐起始值 (CubeMX 默认) |
| 2 | /4 | 兼容模式 |

## 实测数据 (闪迪 8GB SDHC, FK743M2-IIT6)

| 源时钟 | ClockDiv | 总线频率 | 写入 | 读取 | 连续读 | 稳定性 |
|--------|----------|----------|------|------|--------|--------|
| 48MHz | 2 | 12MHz | 4.57 MB/s | 5.33 MB/s | 100/100 | PASS |
| 48MHz | 1 | 24MHz | 6.40 MB/s | 10.67 MB/s | 100/100 | **PASS (推荐)** |
| 48MHz | 0 | 48MHz | 10.67 MB/s | 16.00 MB/s | 25/100 | FAIL |
| 200MHz | 2 | 50MHz | 10.67 MB/s | 16.00 MB/s | 41/100 | FAIL |

**结论：闪迪卡在 24MHz 最稳定，48MHz 及以上出现 RX_OVERRUN，连续读不稳定。**

## 错误分析

| 错误 | 含义 | 原因 |
|------|------|------|
| `RX_OVERRUN` (0x20) | 接收 FIFO 溢出 | 数据来得太快，CPU/IDMA 来不及取走 |
| `CMD_TIMEOUT\|ILLEGAL_CMD` (0x2004) | 命令超时 + 非法命令 | FIFO 溢出后卡状态机被打乱，后续命令不响应 |

发生 `RX_OVERRUN` 后，`HAL_SD_Abort()` 无法恢复传输状态，卡将持续返回 `CMD_TIMEOUT`，需要重新初始化 SD 卡才能恢复。

## 已知问题

- **闪迪大容量卡**：对高速模式兼容性较差，24MHz 是推荐上限，超过即可能出现连续读失败
- **金士顿卡**：据网络反馈对高速模式兼容性更好，部分型号可在 50MHz 稳定工作
- **ClockDiv=0**：SDMMC_CK 计算为 0，实际寄存器行为等同 /1（不分频），但应从 1 开始设置
