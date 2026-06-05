#include "vendor.h"
#include "vendor_flash.h"
#include "qspi_w25q64.h"
#include "quadspi.h"
#include "stm32h7xx_hal_flash.h"
#include "stm32h7xx_hal_flash_ex.h"
#include <string.h>

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static int is_internal(uint32_t addr) {
    return (addr >= VENDOR_FLASH_INT_BASE &&
            addr <  VENDOR_FLASH_INT_BASE + VENDOR_FLASH_INT_SIZE);
}

static int is_external(uint32_t addr) {
    return (addr >= VENDOR_FLASH_EXT_BASE &&
            addr <  VENDOR_FLASH_EXT_BASE + VENDOR_FLASH_EXT_SIZE);
}

//--------------------------------------------------------------------+
// 外部接口 — Flash 信息查询
//--------------------------------------------------------------------+

void vendor_flash_get_info(vendor_flash_info_t *info) {
    // 退出内存映射模式才能发间接指令
    HAL_QSPI_Abort(&hqspi);
    uint32_t id = qspi_w25q64_read_id();
    qspi_w25q64_memory_mapped_mode();

    if (id == QSPI_W25Q64_FLASH_ID) {
        info->present = 1;
        info->size    = QSPI_W25Q64_FLASH_SIZE;
        info->mid     = (id >> 16) & 0xFF;
        info->did     = id & 0xFFFF;
    } else {
        info->present = 0;
        info->size    = 0;
        info->mid     = 0;
        info->did     = 0;
    }
}

//--------------------------------------------------------------------+
// 外部接口 — 擦除 / 写入 (按地址自动分发)
//--------------------------------------------------------------------+

int vendor_flash_erase(uint32_t addr, uint32_t size) {
    if (is_internal(addr)) return vendor_flash_erase_internal(addr, size);
    if (is_external(addr)) return vendor_flash_erase_external(addr, size);
    return -1;
}

int vendor_flash_write(uint32_t addr, const uint8_t *data, uint32_t size) {
    if (is_internal(addr)) return vendor_flash_write_internal(addr, data, size);
    if (is_external(addr)) return vendor_flash_write_external(addr, data, size);
    return -1;
}

//--------------------------------------------------------------------+
// 外部接口 — 内部 Flash (128KB 扇区, 256-bit FlashWord 编程)
//--------------------------------------------------------------------+

int vendor_flash_erase_internal(uint32_t addr, uint32_t size) {
    if (size == 0) return 0;
    if (addr % VENDOR_FLASH_INT_SECTOR_SIZE != 0) return -1;

    uint32_t first_sector = (addr - VENDOR_FLASH_INT_BASE) / VENDOR_FLASH_INT_SECTOR_SIZE;
    uint32_t nb_sectors   = (size + VENDOR_FLASH_INT_SECTOR_SIZE - 1) / VENDOR_FLASH_INT_SECTOR_SIZE;

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Banks        = FLASH_BANK_1,
        .Sector       = first_sector,
        .NbSectors    = nb_sectors,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    uint32_t sector_error = 0;
    HAL_StatusTypeDef ret = HAL_FLASHEx_Erase(&erase, &sector_error);

    HAL_FLASH_Lock();
    return (ret == HAL_OK) ? 0 : -1;
}

int vendor_flash_write_internal(uint32_t addr, const uint8_t *data, uint32_t size) {
    if (size == 0) return 0;
    if (addr % 32 != 0 || size % 32 != 0) return -1;

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < size; i += 32) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              addr + i, (uint32_t)(data + i)) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

//--------------------------------------------------------------------+
// 外部接口 — 外部 Flash (W25Q64, 4KB/32KB/64KB 扇区, 256B 页)
//   调用前需退出内存映射模式，调用后由上层恢复
//--------------------------------------------------------------------+

int vendor_flash_erase_external(uint32_t addr, uint32_t size) {
    if (size == 0) return 0;

    uint32_t off     = addr - VENDOR_FLASH_EXT_BASE;
    uint32_t end_off = off + size;

    while (off < end_off) {
        int8_t ret;
        uint32_t remaining = end_off - off;

        if ((off % (64 * 1024) == 0) && remaining >= 64 * 1024) {
            ret = qspi_w25q64_block_erase_64k(off);
            if (ret != QSPI_W25Q64_OK) return -1;
            off += 64 * 1024;
        } else if ((off % (32 * 1024) == 0) && remaining >= 32 * 1024) {
            ret = qspi_w25q64_block_erase_32k(off);
            if (ret != QSPI_W25Q64_OK) return -1;
            off += 32 * 1024;
        } else {
            ret = qspi_w25q64_sector_erase(off);
            if (ret != QSPI_W25Q64_OK) return -1;
            off += 4 * 1024;
        }
    }

    return 0;
}

int vendor_flash_write_external(uint32_t addr, const uint8_t *data, uint32_t size) {
    if (size == 0) return 0;

    uint32_t off = addr - VENDOR_FLASH_EXT_BASE;
    return (qspi_w25q64_write_buffer((uint8_t *)data, off, size) == QSPI_W25Q64_OK) ? 0 : -1;
}
