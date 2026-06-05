#ifndef BSP_QSPI_W25Q64_H
#define BSP_QSPI_W25Q64_H

#include "stm32h7xx_hal.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 返回值宏
//--------------------------------------------------------------------+
#define QSPI_W25Q64_OK                   0
#define QSPI_W25Q64_ERROR_INIT          -1
#define QSPI_W25Q64_ERROR_WRITE_ENABLE  -2
#define QSPI_W25Q64_ERROR_AUTOPOLLING   -3
#define QSPI_W25Q64_ERROR_ERASE         -4
#define QSPI_W25Q64_ERROR_TRANSMIT      -5
#define QSPI_W25Q64_ERROR_MEMORY_MAPPED -6

//--------------------------------------------------------------------+
// 指令码
//--------------------------------------------------------------------+
#define QSPI_W25Q64_CMD_ENABLE_RESET          0x66
#define QSPI_W25Q64_CMD_RESET_DEVICE          0x99
#define QSPI_W25Q64_CMD_JEDEC_ID              0x9F
#define QSPI_W25Q64_CMD_WRITE_ENABLE          0x06
#define QSPI_W25Q64_CMD_SECTOR_ERASE          0x20
#define QSPI_W25Q64_CMD_BLOCK_ERASE_32K       0x52
#define QSPI_W25Q64_CMD_BLOCK_ERASE_64K       0xD8
#define QSPI_W25Q64_CMD_CHIP_ERASE            0xC7
#define QSPI_W25Q64_CMD_QUAD_INPUT_PAGE_PROG  0x32
#define QSPI_W25Q64_CMD_FAST_READ_QUAD_IO     0xEB
#define QSPI_W25Q64_CMD_READ_STATUS_REG1      0x05

//--------------------------------------------------------------------+
// 状态寄存器位
//--------------------------------------------------------------------+
#define QSPI_W25Q64_STATUS_REG1_BUSY  0x01
#define QSPI_W25Q64_STATUS_REG1_WEL   0x02

//--------------------------------------------------------------------+
// 芯片参数
//--------------------------------------------------------------------+
#define QSPI_W25Q64_PAGE_SIZE             256
#define QSPI_W25Q64_FLASH_SIZE            (8 * 1024 * 1024)
#define QSPI_W25Q64_FLASH_ID              0x00EF4017
#define QSPI_W25Q64_CHIP_ERASE_TIMEOUT_MAX  100000U
#define QSPI_W25Q64_MEM_ADDR              0x90000000

//--------------------------------------------------------------------+
// 引脚配置
//--------------------------------------------------------------------+
#define QSPI_W25Q64_CLK_PIN          GPIO_PIN_2
#define QSPI_W25Q64_CLK_PORT         GPIOB
#define QSPI_W25Q64_CLK_AF           GPIO_AF9_QUADSPI
#define QSPI_W25Q64_CLK_CLK_ENABLE   __HAL_RCC_GPIOB_CLK_ENABLE()

#define QSPI_W25Q64_BK1_NCS_PIN          GPIO_PIN_6
#define QSPI_W25Q64_BK1_NCS_PORT         GPIOB
#define QSPI_W25Q64_BK1_NCS_AF           GPIO_AF10_QUADSPI
#define QSPI_W25Q64_BK1_NCS_CLK_ENABLE   __HAL_RCC_GPIOB_CLK_ENABLE()

#define QSPI_W25Q64_BK1_IO0_PIN          GPIO_PIN_11
#define QSPI_W25Q64_BK1_IO0_PORT         GPIOD
#define QSPI_W25Q64_BK1_IO0_AF           GPIO_AF9_QUADSPI
#define QSPI_W25Q64_BK1_IO0_CLK_ENABLE   __HAL_RCC_GPIOD_CLK_ENABLE()

#define QSPI_W25Q64_BK1_IO1_PIN          GPIO_PIN_12
#define QSPI_W25Q64_BK1_IO1_PORT         GPIOD
#define QSPI_W25Q64_BK1_IO1_AF           GPIO_AF9_QUADSPI
#define QSPI_W25Q64_BK1_IO1_CLK_ENABLE   __HAL_RCC_GPIOD_CLK_ENABLE()

#define QSPI_W25Q64_BK1_IO2_PIN          GPIO_PIN_2
#define QSPI_W25Q64_BK1_IO2_PORT         GPIOE
#define QSPI_W25Q64_BK1_IO2_AF           GPIO_AF9_QUADSPI
#define QSPI_W25Q64_BK1_IO2_CLK_ENABLE   __HAL_RCC_GPIOE_CLK_ENABLE()

#define QSPI_W25Q64_BK1_IO3_PIN          GPIO_PIN_13
#define QSPI_W25Q64_BK1_IO3_PORT         GPIOD
#define QSPI_W25Q64_BK1_IO3_AF           GPIO_AF9_QUADSPI
#define QSPI_W25Q64_BK1_IO3_CLK_ENABLE   __HAL_RCC_GPIOD_CLK_ENABLE()

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

int8_t    qspi_w25q64_init(void);
int8_t    qspi_w25q64_reset(void);
uint32_t  qspi_w25q64_read_id(void);
int8_t    qspi_w25q64_memory_mapped_mode(void);

int8_t    qspi_w25q64_sector_erase(uint32_t sector_address);
int8_t    qspi_w25q64_block_erase_32k(uint32_t sector_address);
int8_t    qspi_w25q64_block_erase_64k(uint32_t sector_address);
int8_t    qspi_w25q64_chip_erase(void);

int8_t    qspi_w25q64_write_page(uint8_t *buf, uint32_t addr, uint16_t size);
int8_t    qspi_w25q64_write_buffer(uint8_t *data, uint32_t addr, uint32_t size);
int8_t    qspi_w25q64_read_buffer(uint8_t *buf, uint32_t addr, uint32_t size);

//--------------------------------------------------------------------+
// 单元测试 (USE_BSP_UNIT_TESTING 宏控制)
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void qspi_w25q64_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
