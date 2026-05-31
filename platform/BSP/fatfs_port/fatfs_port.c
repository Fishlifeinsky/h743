#include "fatfs_port.h"
#include "ff.h"
#include "sd.h"

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static int g_sd_ready = 0;

// SD 卡信息缓存 (fatfs_port_sd_init 时填充)
static HAL_SD_CardInfoTypeDef g_card_info;

// FatFs 文件系统对象 (只支持单卷)
static FATFS g_fs;

//--------------------------------------------------------------------+
// FatFs 固定名称回调 — 时间戳 (FF_FS_NORTC=0 时 FatFs 自动调用)
//--------------------------------------------------------------------+

DWORD get_fattime(void) {
    // 无 RTC, 返回固定时间: 2025-01-01 00:00:00
    return ((DWORD)(2025 - 1980) << 25)
         | ((DWORD)1 << 21)
         | ((DWORD)1 << 16);
}

//--------------------------------------------------------------------+
// 外部接口 — SD 卡底层操作
//--------------------------------------------------------------------+

int fatfs_port_sd_init(void) {
    // sd_init() 已在 main.c 中提前调用, 此处直接获取信息

    if (!sd_is_ready()) {
        DEBUG_PRINT("FATFS: sd not ready\r\n");
        g_sd_ready = 0;
        return -1;
    }

    if (sd_get_info(&g_card_info) != 0) {
        DEBUG_PRINT("FATFS: get card info failed\r\n");
        g_sd_ready = 0;
        return -1;
    }

    g_sd_ready = 1;
    DEBUG_PRINT("FATFS: sd init ok\r\n");
    return 0;
}

int fatfs_port_sd_status(void) {
    if (!g_sd_ready) return -1;

    HAL_SD_CardStateTypeDef state;
    if (sd_get_state(&state) != 0) return -1;
    if (state == HAL_SD_CARD_ERROR || state == HAL_SD_CARD_DISCONNECTED) {
        return -1;
    }
    return 0;
}

int fatfs_port_sd_read(uint8_t *buf, uint32_t sector, uint32_t count) {
    if (!g_sd_ready) return -1;

    if (sd_read(buf, sector, count, 1000) != 0) {
        DEBUG_PRINT("FATFS: read sector=%lu count=%lu failed\r\n", sector, count);
        return -1;
    }
    return 0;
}

int fatfs_port_sd_write(const uint8_t *buf, uint32_t sector, uint32_t count) {
    if (!g_sd_ready) return -1;

    if (sd_write(buf, sector, count, 1000) != 0) {
        DEBUG_PRINT("FATFS: write sector=%lu count=%lu failed\r\n", sector, count);
        return -1;
    }
    return 0;
}

int fatfs_port_sd_ioctl(uint8_t cmd, void *buf) {
    if (!g_sd_ready) return -1;

    switch (cmd) {
    case 0:  // CTRL_SYNC
        return 0;

    case 1:  // GET_SECTOR_COUNT
        *(uint32_t *)buf = g_card_info.BlockNbr;
        return 0;

    case 2:  // GET_SECTOR_SIZE
        *(uint16_t *)buf = (uint16_t)g_card_info.BlockSize;
        return 0;

    case 3:  // GET_BLOCK_SIZE
        *(uint32_t *)buf = 1;  // SD 卡单扇区擦除
        return 0;

    default:
        return -1;
    }
}

//--------------------------------------------------------------------+
// 静态函数 — 调试用: hex dump 扇区 0
//--------------------------------------------------------------------+

static void fatfs_port_dump_sector0(void) {
    uint8_t buf[512];
    if (fatfs_port_sd_read(buf, 0, 1) != 0) {
        DEBUG_PRINT("FATFS: dump sector 0 failed\r\n");
        return;
    }

    DEBUG_PRINT("FATFS: sector 0 dump (first 64 bytes):\r\n");
    for (int i = 0; i < 64; i += 16) {
        DEBUG_PRINT("  %02X %02X %02X %02X %02X %02X %02X %02X "
                    "%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                    buf[i+0],  buf[i+1],  buf[i+2],  buf[i+3],
                    buf[i+4],  buf[i+5],  buf[i+6],  buf[i+7],
                    buf[i+8],  buf[i+9],  buf[i+10], buf[i+11],
                    buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
    }
}

//--------------------------------------------------------------------+
// 外部接口 — FatFs 挂载 / 信息
//--------------------------------------------------------------------+

int fatfs_port_init(void) {
    FRESULT res = f_mount(&g_fs, "", 1);
    if (res != FR_OK) {
        DEBUG_PRINT("FATFS: mount failed (%d)\r\n", res);
        fatfs_port_dump_sector0();
        return -1;
    }

    DEBUG_PRINT("FATFS: mount ok\r\n");
    return 0;
}

void fatfs_port_get_info(void) {
    if (!g_sd_ready) {
        DEBUG_PRINT("FATFS: no card info\r\n");
        return;
    }

    DEBUG_PRINT("FATFS: CardType=%lu, BlockSize=%lu, BlockNbr=%lu\r\n",
                g_card_info.CardType, g_card_info.BlockSize, g_card_info.BlockNbr);
    DEBUG_PRINT("FATFS: LogBlockSize=%lu, LogBlockNbr=%lu\r\n",
                g_card_info.LogBlockSize, g_card_info.LogBlockNbr);
}
