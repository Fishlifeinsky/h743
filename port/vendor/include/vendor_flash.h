#ifndef BSP_VENDOR_FLASH_H_
#define BSP_VENDOR_FLASH_H_

#include "vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// Flash 操作 — 按地址自动分发内部/外部 Flash
//--------------------------------------------------------------------+

// 获取外部 Flash 信息 (JEDEC ID)
void vendor_flash_get_info(vendor_flash_info_t *info);

// 擦除: addr 指定的 VENDOR_FLASH_INT_BASE 或 VENDOR_FLASH_EXT_BASE 区域
int vendor_flash_erase(uint32_t addr, uint32_t size);

// 写入: addr 指定的区域
int vendor_flash_write(uint32_t addr, const uint8_t *data, uint32_t size);

// 内部 Flash 直接操作 (调用者确保地址范围正确)
int vendor_flash_erase_internal(uint32_t addr, uint32_t size);
int vendor_flash_write_internal(uint32_t addr, const uint8_t *data, uint32_t size);

// 外部 Flash 直接操作
int vendor_flash_erase_external(uint32_t addr, uint32_t size);
int vendor_flash_write_external(uint32_t addr, const uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
