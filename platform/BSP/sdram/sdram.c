#include "sdram.h"
#include "fmc.h"

// ~1ms busy-wait @ 240MHz HCLK, 不依赖中断/uwTick
#define SDRAM_BUSY_DELAY_1MS() do { \
    for (volatile uint32_t _d = 0; _d < 60000; _d++) { __NOP(); } \
} while(0)

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void sdram_init_sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *command)
{
    __IO uint32_t tmpmrd = 0;

    // 开启 SDRAM 时钟
    command->CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    command->CommandTarget          = FMC_COMMAND_TARGET_BANK;
    command->AutoRefreshNumber      = 1;
    command->ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, command, SDRAM_TIMEOUT);
    // SDRAM_BUSY_DELAY_1MS();
    HAL_Delay(1);
    // 预充电命令
    command->CommandMode            = FMC_SDRAM_CMD_PALL;
    command->CommandTarget          = FMC_COMMAND_TARGET_BANK;
    command->AutoRefreshNumber      = 1;
    command->ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, command, SDRAM_TIMEOUT);

    // 自动刷新
    command->CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    command->CommandTarget          = FMC_COMMAND_TARGET_BANK;
    command->AutoRefreshNumber      = 8;
    command->ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, command, SDRAM_TIMEOUT);

    // 加载模式寄存器
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2         |
                          SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                          SDRAM_MODEREG_CAS_LATENCY_3           |
                          SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                          SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    command->CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    command->CommandTarget          = FMC_COMMAND_TARGET_BANK;
    command->AutoRefreshNumber      = 1;
    command->ModeRegisterDefinition = tmpmrd;

    HAL_SDRAM_SendCommand(hsdram, command, SDRAM_TIMEOUT);

    // 配置刷新率
    HAL_SDRAM_ProgramRefreshRate(hsdram, 918);
}
