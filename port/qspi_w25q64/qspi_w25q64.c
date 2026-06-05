#include "qspi_w25q64.h"
#include "quadspi.h"

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static __IO uint8_t g_rx_status = 0;

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static int8_t qspi_w25q64_auto_polling_ready(void)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.Instruction       = QSPI_W25Q64_CMD_READ_STATUS_REG1;

    s_config.Match           = 0;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.StatusBytesSize = 1;
    s_config.Mask            = QSPI_W25Q64_STATUS_REG1_BUSY;

    if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

static int8_t qspi_w25q64_write_enable(void)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode        = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    s_command.Instruction        = QSPI_W25Q64_CMD_WRITE_ENABLE;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }

    s_config.Match           = 0x02;
    s_config.Mask            = QSPI_W25Q64_STATUS_REG1_WEL;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction    = QSPI_W25Q64_CMD_READ_STATUS_REG1;
    s_command.DataMode       = QSPI_DATA_1_LINE;
    s_command.NbData         = 1;

    if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

int8_t qspi_w25q64_init(void)
{
    uint32_t device_id;

    qspi_w25q64_reset();
    device_id = qspi_w25q64_read_id();

    if (device_id == QSPI_W25Q64_FLASH_ID)
    {
        DEBUG_PRINT("W25Q64 OK, flash ID:%lX\r\n", device_id);
        return QSPI_W25Q64_OK;
    }
    else
    {
        DEBUG_PRINT("W25Q64 ERROR!!!!!  ID:%lX\r\n", device_id);
        return QSPI_W25Q64_ERROR_INIT;
    }
}

int8_t qspi_w25q64_reset(void)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.Instruction       = QSPI_W25Q64_CMD_ENABLE_RESET;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_INIT;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }

    s_command.Instruction = QSPI_W25Q64_CMD_RESET_DEVICE;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_INIT;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

uint32_t qspi_w25q64_read_id(void)
{
    QSPI_CommandTypeDef s_command;
    uint8_t  rx_buf[3];
    uint32_t id;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 3;
    s_command.Instruction       = QSPI_W25Q64_CMD_JEDEC_ID;

    HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    HAL_QSPI_Receive(&hqspi, rx_buf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

    id = (rx_buf[0] << 16) | (rx_buf[1] << 8) | rx_buf[2];
    return id;
}

int8_t qspi_w25q64_memory_mapped_mode(void)
{
    QSPI_CommandTypeDef      s_command;
    QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 6;
    s_command.Instruction       = QSPI_W25Q64_CMD_FAST_READ_QUAD_IO;

    s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    s_mem_mapped_cfg.TimeOutPeriod     = 0;

    qspi_w25q64_reset();

    if (HAL_QSPI_MemoryMapped(&hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_MEMORY_MAPPED;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_sector_erase(uint32_t sector_address)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_1_LINE;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    s_command.Address            = sector_address;
    s_command.Instruction        = QSPI_W25Q64_CMD_SECTOR_ERASE;

    if (qspi_w25q64_write_enable() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_ERASE;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_block_erase_32k(uint32_t sector_address)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_1_LINE;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    s_command.Address            = sector_address;
    s_command.Instruction        = QSPI_W25Q64_CMD_BLOCK_ERASE_32K;

    if (qspi_w25q64_write_enable() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_ERASE;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_block_erase_64k(uint32_t sector_address)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_1_LINE;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    s_command.Address            = sector_address;
    s_command.Instruction        = QSPI_W25Q64_CMD_BLOCK_ERASE_64K;

    if (qspi_w25q64_write_enable() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_ERASE;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_chip_erase(void)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize        = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_NONE;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    s_command.Instruction        = QSPI_W25Q64_CMD_CHIP_ERASE;

    if (qspi_w25q64_write_enable() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_ERASE;
    }

    s_config.Match           = 0;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.StatusBytesSize = 1;
    s_config.Mask            = QSPI_W25Q64_STATUS_REG1_BUSY;

    s_command.Instruction    = QSPI_W25Q64_CMD_READ_STATUS_REG1;
    s_command.DataMode       = QSPI_DATA_1_LINE;
    s_command.NbData         = 1;

    if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config, QSPI_W25Q64_CHIP_ERASE_TIMEOUT_MAX) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_write_page(uint8_t *buf, uint32_t addr, uint16_t size)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.NbData            = size;
    s_command.Address           = addr;
    s_command.Instruction       = QSPI_W25Q64_CMD_QUAD_INPUT_PAGE_PROG;

    if (qspi_w25q64_write_enable() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_WRITE_ENABLE;
    }
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_TRANSMIT;
    }
    if (HAL_QSPI_Transmit(&hqspi, buf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_TRANSMIT;
    }
    if (qspi_w25q64_auto_polling_ready() != QSPI_W25Q64_OK)
    {
        return QSPI_W25Q64_ERROR_AUTOPOLLING;
    }
    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_write_buffer(uint8_t *data, uint32_t addr, uint32_t size)
{
    uint32_t end_addr, current_size, current_addr;
    uint8_t *write_data;

    current_size = QSPI_W25Q64_PAGE_SIZE - (addr % QSPI_W25Q64_PAGE_SIZE);

    if (current_size > size)
    {
        current_size = size;
    }

    current_addr = addr;
    end_addr     = addr + size;
    write_data   = data;

    do
    {
        if (qspi_w25q64_write_page(write_data, current_addr, current_size) != QSPI_W25Q64_OK)
        {
            return QSPI_W25Q64_ERROR_TRANSMIT;
        }
        else
        {
            current_addr += current_size;
            write_data   += current_size;
            current_size  = ((current_addr + QSPI_W25Q64_PAGE_SIZE) > end_addr)
                                ? (end_addr - current_addr)
                                : QSPI_W25Q64_PAGE_SIZE;
        }
    }
    while (current_addr < end_addr);

    return QSPI_W25Q64_OK;
}

int8_t qspi_w25q64_read_buffer(uint8_t *buf, uint32_t addr, uint32_t size)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 6;
    s_command.NbData            = size;
    s_command.Address           = addr;
    s_command.Instruction       = QSPI_W25Q64_CMD_FAST_READ_QUAD_IO;

    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_TRANSMIT;
    }

    if (HAL_QSPI_Receive_DMA(&hqspi, buf) != HAL_OK)
    {
        return QSPI_W25Q64_ERROR_TRANSMIT;
    }

    while (g_rx_status == 0);
    g_rx_status = 0;

    return QSPI_W25Q64_OK;
}

//--------------------------------------------------------------------+
// HAL 回调
//--------------------------------------------------------------------+

void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
    g_rx_status = 1;
}
