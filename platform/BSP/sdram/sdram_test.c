#include "sdram.h"
#include "BSP/config.h"

#if USE_BSP_UNIT_TESTING

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void sdram_test(void)
{
    uint32_t i;
    uint16_t read_data_16;
    uint8_t  read_data_8;
    uint32_t t_start, t_end, t_elapsed;
    float    speed;

    DEBUG_PRINT("\r\n*****************************************************************************************************\r\n");
    DEBUG_PRINT("\r\n进行速度测试>>>\r\n");

    // ---- 16位数据宽度写入 ----
    t_start = HAL_GetTick();

    for (i = 0; i < SDRAM_SIZE / 2; i++) {
        *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i) = (uint16_t)i;
    }
    t_end     = HAL_GetTick();
    t_elapsed = t_end - t_start;
    speed     = (float)SDRAM_SIZE / 1024 / 1024 / t_elapsed * 1000;

    DEBUG_PRINT("\r\n以16位数据宽度写入数据，大小：%lu MB，耗时: %lu ms, 写入速度：%.2f MB/s\r\n",
           SDRAM_SIZE / 1024 / 1024, t_elapsed, speed);

    // ---- 16位数据宽度读取 ----
    t_start = HAL_GetTick();

    for (i = 0; i < SDRAM_SIZE / 2; i++) {
        read_data_16 = *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i);
    }
    t_end     = HAL_GetTick();
    t_elapsed = t_end - t_start;
    speed     = (float)SDRAM_SIZE / 1024 / 1024 / t_elapsed * 1000;

    DEBUG_PRINT("\r\n读取数据完毕，大小：%lu MB，耗时: %lu ms, 读取速度：%.2f MB/s\r\n",
           SDRAM_SIZE / 1024 / 1024, t_elapsed, speed);

    // ---- 数据校验 ----
    DEBUG_PRINT("\r\n*****************************************************************************************************\r\n");
    DEBUG_PRINT("\r\n进行数据校验>>>\r\n");

    for (i = 0; i < SDRAM_SIZE / 2; i++) {
        read_data_16 = *(__IO uint16_t *)(SDRAM_BANK_ADDR + 2 * i);
        if (read_data_16 != (uint16_t)i) {
            DEBUG_PRINT("\r\nSDRAM测试失败！！\r\n");
            return;
        }
    }

    DEBUG_PRINT("\r\n16位数据宽度读写通过，以8位数据宽度写入数据\r\n");
    for (i = 0; i < 255; i++) {
        *(__IO uint8_t *)(SDRAM_BANK_ADDR + i) = (uint8_t)i;
    }
    DEBUG_PRINT("写入完毕，读取数据并比较...\r\n");
    for (i = 0; i < 255; i++) {
        read_data_8 = *(__IO uint8_t *)(SDRAM_BANK_ADDR + i);
        if (read_data_8 != (uint8_t)i) {
            DEBUG_PRINT("8位数据宽度读写测试失败！！\r\n");
            DEBUG_PRINT("请检查NBL0和NBL1的连接\r\n");
            return;
        }
    }
    DEBUG_PRINT("8位数据宽度读写通过\r\n");
    DEBUG_PRINT("SDRAM读写测试通过，系统正常\r\n");
}

#endif // USE_BSP_UNIT_TESTING
