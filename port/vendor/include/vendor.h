#ifndef BSP_VENDOR_H_
#define BSP_VENDOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 协议常量
//--------------------------------------------------------------------+
#define VENDOR_PROTO_PREFIX         "VB+"
#define VENDOR_PROTO_SUFFIX         "\r\n"
#define VENDOR_PROTO_PREFIX_LEN     3
#define VENDOR_PROTO_SUFFIX_LEN     2

#define VENDOR_CMD_BUFF             "BUFF"
#define VENDOR_CMD_FLASH            "FLASH"
#define VENDOR_CMD_LOAD             "LOAD"
#define VENDOR_CMD_DATA             "DATA"
#define VENDOR_CMD_ABORT            "ABORT"

#define VENDOR_STATUS_OK            "OK"
#define VENDOR_STATUS_ERR           "ERR"

//--------------------------------------------------------------------+
// 缓冲区配置 (基于 tusb_config.h 中 CFG_TUD_VENDOR_RX/TX_BUFSIZE)
//--------------------------------------------------------------------+
#define VENDOR_RX_BUF_DEFAULT       512
#define VENDOR_RX_BUF_MAX           4096
#define VENDOR_TX_BUF_SIZE          512

//--------------------------------------------------------------------+
// Flash 地址范围
//--------------------------------------------------------------------+
#define VENDOR_FLASH_INT_BASE       0x08000000U
#define VENDOR_FLASH_INT_SIZE       (2 * 1024 * 1024)    // 2MB 内部 Flash
#define VENDOR_FLASH_EXT_BASE       0x90000000U          // QSPI 内存映射
#define VENDOR_FLASH_EXT_SIZE       (8 * 1024 * 1024)    // 8MB W25Q64

// 内部 Flash 扇区大小 (STM32H743 单 Bank: 8 × 128KB)
#define VENDOR_FLASH_INT_SECTOR_SIZE  (128 * 1024)

// 外部 Flash 扇区大小 (W25Q64: 4KB)
#define VENDOR_FLASH_EXT_SECTOR_SIZE  (4 * 1024)

//--------------------------------------------------------------------+
// 外部 Flash 信息
//--------------------------------------------------------------------+
typedef struct {
    uint8_t  present;       // 1 = 存在, 0 = 不存在
    uint32_t size;          // 容量（字节）
    uint32_t mid;           // 厂商 ID (Manufacturer ID)
    uint32_t did;           // 设备 ID (Device ID)
} vendor_flash_info_t;

//--------------------------------------------------------------------+
// 公开 API
//--------------------------------------------------------------------+
void vendor_init(void);
void vendor_poll(void);

#ifdef __cplusplus
}
#endif

#endif
