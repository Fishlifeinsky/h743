#include "usb_msc.h"
#include "tusb.h"
#include "sd.h"
#include <string.h>

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

// SD 卡信息缓存
static HAL_SD_CardInfoTypeDef g_card_info;
static volatile bool           g_card_ready = false;
static volatile bool           g_ejected    = false;

// 单扇区缓存 (避免同一 LBA 的多次部分读取反复读 SD)
static uint32_t g_cache_lba = 0xFFFFFFFF;
static uint8_t  g_cache_buf[512] TU_ATTR_ALIGNED(4);


//--------------------------------------------------------------------+
// TinyUSB MSC 回调 (固定名称, 由 TinyUSB 库内部调用)
//--------------------------------------------------------------------+

// 读取容量: 扇区数 + 扇区大小
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void)lun;
    if (g_card_ready) {
        *block_count = g_card_info.BlockNbr;
        *block_size  = (uint16_t)g_card_info.BlockSize;
    } else {
        *block_count = 0;
        *block_size  = 0;
    }
}

// SCSI INQUIRY 响应: 厂商/产品/版本
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                        uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;
    const char vid[] = "STM32H7";
    const char pid[] = "SD Mass Storage";
    const char rev[] = "1.0";

    memcpy(vendor_id,  vid, sizeof(vid));
    memcpy(product_id, pid, sizeof(pid));
    memcpy(product_rev, rev, sizeof(rev));
}

// 介质就绪检测
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;
    return g_card_ready && !g_ejected;
}

// Start/Stop 与介质弹出
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition,
                           bool start, bool load_eject) {
    (void)lun;
    (void)power_condition;

    if (load_eject) {
        if (!start) {
            // Eject: 主机请求弹出介质
            g_ejected = true;
        } else {
            // Load: 主机请求加载介质
            g_ejected = false;
        }
    }
    return true;
}

// 读扇区 (SCSI READ10)
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
    (void)lun;

    if (!g_card_ready || g_ejected) return -1;

    uint32_t block_size = g_card_info.BlockSize;

    // 全对齐快速路径: 直接 DMA, 绕过缓存
    if (offset == 0 && (bufsize % block_size) == 0) {
        g_cache_lba = 0xFFFFFFFF; // 使缓存失效
        if (sd_read((uint8_t *)buffer, lba, bufsize / block_size, 2000) != 0) {
            DEBUG_PRINT("MSC: read10 lba=%lu failed\r\n", lba);
            return -1;
        }
        return (int32_t)bufsize;
    }

    // 部分块读取: 走缓存, 避免同一 LBA 反复读 SD
    if (g_cache_lba != lba) {
        if (sd_read(g_cache_buf, lba, 1, 2000) != 0) {
            g_cache_lba = 0xFFFFFFFF;
            DEBUG_PRINT("MSC: read10 lba=%lu failed\r\n", lba);
            return -1;
        }
        g_cache_lba = lba;
    }

    uint32_t copy_len = block_size - offset;
    if (copy_len > bufsize) copy_len = bufsize;
    memcpy(buffer, g_cache_buf + offset, copy_len);
    return (int32_t)copy_len;
}

// 写扇区 (SCSI WRITE10)
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize) {
    (void)lun;

    if (!g_card_ready || g_ejected) return -1;

    uint32_t block_size = g_card_info.BlockSize;

    // 全对齐快速路径
    if (offset == 0 && (bufsize % block_size) == 0) {
        g_cache_lba = 0xFFFFFFFF; // 写穿透, 使缓存失效
        if (sd_write(buffer, lba, bufsize / block_size, 2000) != 0) {
            DEBUG_PRINT("MSC: write10 lba=%lu failed\r\n", lba);
            return -1;
        }
        return (int32_t)bufsize;
    }

    // 部分块写入: 读-改-写 (single block only for FS USB)
    uint8_t tmp[512] TU_ATTR_ALIGNED(4);
    if (sd_read(tmp, lba, 1, 2000) != 0) {
        g_cache_lba = 0xFFFFFFFF;
        DEBUG_PRINT("MSC: write10 read-back lba=%lu failed\r\n", lba);
        return -1;
    }

    uint32_t mod_len = block_size - offset;
    if (mod_len > bufsize) mod_len = bufsize;
    memcpy(tmp + offset, buffer, mod_len);

    if (sd_write(tmp, lba, 1, 2000) != 0) {
        g_cache_lba = 0xFFFFFFFF;
        DEBUG_PRINT("MSC: write10 lba=%lu failed\r\n", lba);
        return -1;
    }
    g_cache_lba = 0xFFFFFFFF; // 缓存失效
    return (int32_t)mod_len;
}

// SCSI 命令处理 (非 READ10/WRITE10 的其他 SCSI 命令)
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                        void *buffer, uint16_t bufsize) {
    (void)lun;
    (void)scsi_cmd;
    (void)buffer;
    (void)bufsize;
    return -1;  // 不支持的命令, TinyUSB 会 STALL 端点
}

// 写保护检测
bool tud_msc_is_writable_cb(uint8_t lun) {
    (void)lun;
    return g_card_ready && !g_ejected;
}

// 单 LUN
uint8_t tud_msc_get_maxlun_cb(void) {
    return 0;
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void usb_msc_init(void) {
    if (sd_get_info(&g_card_info) != 0) {
        DEBUG_PRINT("MSC: get card info failed\r\n");
        g_card_ready = false;
        return;
    }

    g_card_ready = true;
    g_ejected    = false;

    DEBUG_PRINT("MSC: init ok, BlockNbr=%lu, BlockSize=%lu\r\n",
                g_card_info.BlockNbr, g_card_info.BlockSize);
}

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void usb_msc_test(void) {
    DEBUG_PRINT("\r\n=== USB MSC Test ===\r\n");

    if (!g_card_ready) {
        DEBUG_PRINT("MSC TEST: card not ready\r\n");
        return;
    }

    DEBUG_PRINT("MSC TEST: BlockNbr=%lu, BlockSize=%lu\r\n",
                g_card_info.BlockNbr, g_card_info.BlockSize);
    DEBUG_PRINT("MSC TEST: LUN=0, writable=%d\r\n", tud_msc_is_writable_cb(0));
    DEBUG_PRINT("=== USB MSC Test PASSED ===\r\n");
}
#endif
