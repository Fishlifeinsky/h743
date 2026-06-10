#include "sd.h"
#include "sdmmc.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "semphr.h"
#endif

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static HAL_SD_CardInfoTypeDef g_info;

MODULE_DECLARE(sd,NULL);

#if BSP_FREERTOS_ENABLED
static SemaphoreHandle_t g_mutex;
static StaticSemaphore_t g_mutex_buffer;

#define SD_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
#define SD_UNLOCK() do { if (g_mutex) xSemaphoreGive(g_mutex); } while(0)
#else
#define SD_LOCK()   ((void)0)
#define SD_UNLOCK() ((void)0)
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

int sd_init(void) {
    MODULE_DEPEND_IS_INIT_ASSERT(sd);
    if(!MODULE_DEPEND_IS_INIT(sd)) return -1;
    if(MODULE_IS_INIT(sd)) return 0;

    if (HAL_SD_GetCardInfo(&hsd1, &g_info) != HAL_OK) {
        DEBUG_PRINT("SD: get card info failed\r\n");
        return -1;
    }

    DEBUG_PRINT("SD: CardType=%lu, BlockSize=%lu, BlockNbr=%lu\r\n",
           g_info.CardType, g_info.BlockSize, g_info.BlockNbr);

    // 打印 SDMMC 时钟信息
    uint32_t sdmmc_clk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
    uint32_t sdmmc_ck  = sdmmc_clk / (2 * hsd1.Init.ClockDiv);
    DEBUG_PRINT("SD: SDMMCCLK=%lu Hz, ClockDiv=%lu, SDMMC_CK=%lu Hz\r\n",
           sdmmc_clk, hsd1.Init.ClockDiv, sdmmc_ck);

    // 配置 4-bit 宽总线
    if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK) {
        DEBUG_PRINT("SD: wide bus failed\r\n");
        return -1;
    }

    // 尝试切换到高速模式 (不强制要求成功)
    if (HAL_SD_ConfigSpeedBusOperation(&hsd1, SDMMC_SPEED_MODE_HIGH) != HAL_OK) {
        DEBUG_PRINT("SD: high speed not supported, running at default\r\n");
    }

    MODULE_INIT_DONE(sd);
    return 0;
}

void sd_rtos_init(void) {
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buffer);
#endif
}

int sd_read(uint8_t *buf, uint32_t sector, uint32_t count, uint32_t timeout_ms) {
    if (!MODULE_IS_INIT(sd)) return -1;

    SD_LOCK();
    if (HAL_SD_ReadBlocks(&hsd1, buf, sector, count, timeout_ms) != HAL_OK) {
        SD_UNLOCK();
        return -1;
    }
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    SD_UNLOCK();
    return 0;
}

int sd_write(const uint8_t *buf, uint32_t sector, uint32_t count, uint32_t timeout_ms) {
    if (!MODULE_IS_INIT(sd)) return -1;

    SD_LOCK();
    if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *)buf, sector, count, timeout_ms) != HAL_OK) {
        SD_UNLOCK();
        return -1;
    }
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    SD_UNLOCK();
    return 0;
}

int sd_erase(uint32_t sector_start, uint32_t sector_end) {
    if (!MODULE_IS_INIT(sd)) return -1;

    SD_LOCK();
    if (HAL_SD_Erase(&hsd1, sector_start, sector_end) != HAL_OK) {
        SD_UNLOCK();
        return -1;
    }
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    SD_UNLOCK();
    return 0;
}

int sd_get_info(HAL_SD_CardInfoTypeDef *info) {
    if (!MODULE_IS_INIT(sd) || !info) return -1;
    *info = g_info;
    return 0;
}

int sd_get_state(HAL_SD_CardStateTypeDef *state) {
    if (!MODULE_IS_INIT(sd) || !state) return -1;
    *state = HAL_SD_GetCardState(&hsd1);
    return 0;
}

int sd_is_ready(void) {
    return MODULE_IS_INIT(sd);
}
