#include "sd.h"
#include "sdmmc.h"
#include <string.h>

#if USE_BSP_UNIT_TESTING

#define TEST_NUM_BLOCKS   64
#define TEST_BLOCK_START  0

// 定义在 stm32h7xx_hal_sd.h 中: BLOCKSIZE = 512
#define TEST_BUF_SIZE  ((BLOCKSIZE * TEST_NUM_BLOCKS) / sizeof(uint32_t))

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

// 大缓冲区，用 static 避免爆栈
static uint32_t sd_wbuf[TEST_BUF_SIZE];
static uint32_t sd_rbuf[TEST_BUF_SIZE];

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

// 错误码解码
static const char *sd_err_name(uint32_t err)
{
    if (err == 0) return "NONE";
    static char buf[128];
    buf[0] = '\0';
    #define CHK(b, s) if (err & (b)) { strcat(buf, s); strcat(buf, "|"); }
    CHK(0x00000001, "CMD_CRC")
    CHK(0x00000002, "DATA_CRC")
    CHK(0x00000004, "CMD_TIMEOUT")
    CHK(0x00000008, "DATA_TIMEOUT")
    CHK(0x00000010, "TX_UNDERRUN")
    CHK(0x00000020, "RX_OVERRUN")
    CHK(0x00000040, "ADDR_MISALIGNED")
    CHK(0x00000080, "BLOCK_LEN_ERR")
    CHK(0x00001000, "COM_CRC_FAILED")
    CHK(0x00002000, "ILLEGAL_CMD")
    CHK(0x00004000, "CARD_ECC_FAILED")
    CHK(0x00008000, "CC_ERR")
    CHK(0x00010000, "GENERAL_UNKNOWN")
    CHK(0x80000000, "TIMEOUT")
    #undef CHK
    size_t len = strlen(buf);
    if (len > 0) buf[len - 1] = '\0';
    return buf;
}

// 根据 CardType 返回名称
static const char *card_type_name(uint32_t t)
{
    switch (t) {
        case CARD_SDSC:       return "SDSC";
        case CARD_SDHC_SDXC:  return "SDHC/SDXC";
        case CARD_SECURED:    return "Secure";
        default:              return "?";
    }
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void sd_test(void)
{
    uint32_t t_start, t_end, elapsed;
    float speed_mbps;
    uint32_t data_kb = BLOCKSIZE * TEST_NUM_BLOCKS / 1024;

    // 记录各项指标，用于最终报告
    uint32_t erase_ms = 0;
    uint32_t write_ms = 0;
    float    write_mbps = 0;
    uint32_t read_ms = 0;
    float    read_mbps = 0;
    uint32_t verify_pass = 0;
    // 连续读
    #define CONT_READ_ROUNDS 100
    uint32_t cont_ok = 0, cont_err = 0;
    uint32_t cont_min = 0xFFFFFFFF, cont_max = 0, cont_total = 0;
    float    cont_avg_mbps = 0;
    // 总体
    const char *result = "PASS";

    DEBUG_PRINT("\r\n========== SD 卡单元测试 ==========\r\n");

    // === 1. 擦除 ===
    t_start = HAL_GetTick();
    int st = sd_erase(TEST_BLOCK_START, TEST_BLOCK_START + TEST_NUM_BLOCKS - 1);
    t_end = HAL_GetTick();
    erase_ms = t_end - t_start;

    if (st != 0) {
        DEBUG_PRINT("擦除失败\r\n");
        result = "FAIL";
        goto report;
    }
    DEBUG_PRINT("擦除成功, 擦除耗时: %lu ms\r\n", erase_ms);

    // === 2. 写入 ===
    for (uint32_t i = 0; i < TEST_BUF_SIZE; i++) {
        sd_wbuf[i] = i;
    }
    t_start = HAL_GetTick();
    st = sd_write((uint8_t *)sd_wbuf, TEST_BLOCK_START, TEST_NUM_BLOCKS, 60000);
    t_end = HAL_GetTick();
    write_ms = t_end - t_start;

    if (st != 0) {
        DEBUG_PRINT("写入失败\r\n");
        result = "FAIL";
        goto report;
    }
    write_mbps = (float)BLOCKSIZE * TEST_NUM_BLOCKS / write_ms / 1024.0f;
    DEBUG_PRINT("写入成功, 数据大小: %lu KB, 耗时: %lu ms, 写入速度: %.2f MB/s\r\n",
           data_kb, write_ms, write_mbps);

    // === 3. 单次读取 ===
    t_start = HAL_GetTick();
    st = sd_read((uint8_t *)sd_rbuf, TEST_BLOCK_START, TEST_NUM_BLOCKS, 60000);
    t_end = HAL_GetTick();
    read_ms = t_end - t_start;

    if (st != 0) {
        DEBUG_PRINT("读取失败\r\n");
        result = "FAIL";
        goto report;
    }
    read_mbps = (float)BLOCKSIZE * TEST_NUM_BLOCKS / read_ms / 1024.0f;
    DEBUG_PRINT("读取成功, 数据大小: %lu KB, 耗时: %lu ms, 读取速度: %.2f MB/s\r\n",
           data_kb, read_ms, read_mbps);

    // === 4. 单次校验 ===
    {
        uint32_t mismatch = 0;
        for (uint32_t i = 0; i < TEST_BUF_SIZE; i++) {
            if (sd_wbuf[i] != sd_rbuf[i]) {
                DEBUG_PRINT("校验失败 [%lu]! w=%08lx r=%08lx\r\n",
                       i, sd_wbuf[i], sd_rbuf[i]);
                mismatch = 1;
                break;
            }
        }
        if (mismatch) {
            result = "FAIL";
            goto report;
        }
        verify_pass = 1;
    }
    DEBUG_PRINT("校验通过, SD 卡读写测试成功\r\n");

    // === 5. 连续读 ===
    DEBUG_PRINT("\r\n连续读测试 (%d 轮)...\r\n", CONT_READ_ROUNDS);

    for (uint32_t r = 0; r < CONT_READ_ROUNDS; r++) {
        t_start = HAL_GetTick();
        st = sd_read((uint8_t *)sd_rbuf, TEST_BLOCK_START, TEST_NUM_BLOCKS, 60000);

        HAL_SD_CardStateTypeDef card_state;
        if (sd_get_state(&card_state) != 0) card_state = HAL_SD_CARD_ERROR;

        t_end = HAL_GetTick();
        elapsed = t_end - t_start;
        cont_total += elapsed;

        if (st != 0 || card_state != HAL_SD_CARD_TRANSFER) {
            cont_err++;
            result = "FAIL";
            DEBUG_PRINT("  [%3lu] ERR, %lu ms\r\n", r + 1, elapsed);
            continue;
        }

        if (elapsed < cont_min) cont_min = elapsed;
        if (elapsed > cont_max) cont_max = elapsed;

        // 校验
        uint32_t mismatch = 0;
        for (uint32_t i = 0; i < TEST_BUF_SIZE; i++) {
            if (sd_wbuf[i] != sd_rbuf[i]) {
                mismatch = 1;
                break;
            }
        }
        if (mismatch) {
            cont_err++;
            result = "FAIL";
            DEBUG_PRINT("  [%3lu] 校验失败, %lu ms\r\n", r + 1, elapsed);
        } else {
            cont_ok++;
            DEBUG_PRINT("  [%3lu] OK, %lu ms\r\n", r + 1, elapsed);
        }
    }

    if (cont_ok > 0) {
        cont_avg_mbps = (float)BLOCKSIZE * TEST_NUM_BLOCKS * cont_ok / cont_total / 1024.0f;
    }
    DEBUG_PRINT("连续读结果: 成功 %lu / 失败 %lu\r\n", cont_ok, cont_err);

report:
    // === 测试报告 ===
    {
        HAL_SD_CardInfoTypeDef info;
        uint32_t cap_mb = 0;
        if (sd_get_info(&info) == 0) {
            cap_mb = (uint32_t)((uint64_t)info.BlockNbr * info.BlockSize / 1024 / 1024);
        }
        uint32_t sdmmc_clk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
        uint32_t sdmmc_ck  = sdmmc_clk / (2 * hsd1.Init.ClockDiv);

        DEBUG_PRINT("\r\n");
        DEBUG_PRINT("┌──────────────────────────────┐\r\n");
        DEBUG_PRINT("│     SD 卡单元测试报告        │\r\n");
        DEBUG_PRINT("├──────────────────────────────┤\r\n");
        DEBUG_PRINT("│ 卡类型:    %-10s          │\r\n", card_type_name(info.CardType));
        DEBUG_PRINT("│ 卡容量:    %4lu MB           │\r\n", cap_mb);
        DEBUG_PRINT("│ 总线频率:  %3lu MHz          │\r\n", sdmmc_ck / 1000000);
        DEBUG_PRINT("│                              │\r\n");
        DEBUG_PRINT("│ 测试项目         耗时   速度 │\r\n");
        DEBUG_PRINT("│ ─────────────────────────── │\r\n");
        DEBUG_PRINT("│ 擦除:         %4lu ms        │\r\n", erase_ms);
        DEBUG_PRINT("│ 写入(32KB):   %4lu ms %6.2f MB/s│\r\n", write_ms, write_mbps);
        DEBUG_PRINT("│ 读取(32KB):   %4lu ms %6.2f MB/s│\r\n", read_ms, read_mbps);
        DEBUG_PRINT("│ 连续读(%d轮): %lu/%lu  %6.2f MB/s│\r\n",
               CONT_READ_ROUNDS, cont_ok, cont_ok + cont_err, cont_avg_mbps);
        DEBUG_PRINT("│                              │\r\n");
        DEBUG_PRINT("│ 结果:        %-13s │\r\n", result);
        DEBUG_PRINT("└──────────────────────────────┘\r\n");
    }
}

#endif // USE_BSP_UNIT_TESTING
