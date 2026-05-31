#include "qspi_w25q64.h"
#include <string.h>
#include "BSP/config.h"

#if USE_BSP_UNIT_TESTING

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

#define QSPI_W25Q64_TEST_SIZE  (32 * 1024)

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void qspi_w25q64_test(void)
{
    static uint8_t  write_buf[QSPI_W25Q64_TEST_SIZE];
    static uint8_t  read_buf[QSPI_W25Q64_TEST_SIZE];
    uint32_t        test_addr = 0;
    int8_t          status;
    uint32_t        i;
    uint32_t        t_start, t_end, t_elapsed;
    float           speed;

    // ---- 擦除 32K 块 ----
    t_start = HAL_GetTick();
    status  = qspi_w25q64_block_erase_32k(test_addr);
    t_end   = HAL_GetTick();
    t_elapsed = t_end - t_start;

    if (status == QSPI_W25Q64_OK) {
        DEBUG_PRINT("\r\nW25Q64 擦除成功, 擦除32K字节所需时间: %lu ms\r\n", t_elapsed);
    } else {
        DEBUG_PRINT("\r\n擦除失败!!!!!  错误代码:%d\r\n", status);
        return;
    }

    // ---- 写入 32KB 测试数据 ----
    for (i = 0; i < QSPI_W25Q64_TEST_SIZE; i++) {
        write_buf[i] = i;
    }
    t_start = HAL_GetTick();
    status  = qspi_w25q64_write_buffer(write_buf, test_addr, QSPI_W25Q64_TEST_SIZE);
    t_end   = HAL_GetTick();
    t_elapsed = t_end - t_start;
    speed   = (float)QSPI_W25Q64_TEST_SIZE / 1024 / t_elapsed * 1000;

    if (status == QSPI_W25Q64_OK) {
        DEBUG_PRINT("\r\n写入成功,数据大小：%lu KB, 耗时: %lu ms, 写入速度：%.2f KB/s\r\n",
               QSPI_W25Q64_TEST_SIZE / 1024, t_elapsed, speed);
    } else {
        DEBUG_PRINT("\r\n写入错误!!!!!  错误代码:%d\r\n", status);
        return;
    }

    // ---- 内存映射模式读取 ----
    DEBUG_PRINT("\r\n*****************************************************************************************************\r\n");

    status = qspi_w25q64_memory_mapped_mode();
    if (status == QSPI_W25Q64_OK) {
        DEBUG_PRINT("\r\n进入内存映射模式成功，开始读取>>>>\r\n");
    } else {
        DEBUG_PRINT("\r\n内存映射错误！！  错误代码:%d\r\n", status);
        return;
    }

    t_start = HAL_GetTick();
    memcpy(read_buf, (uint8_t *)QSPI_W25Q64_MEM_ADDR + test_addr, QSPI_W25Q64_TEST_SIZE);
    t_end   = HAL_GetTick();
    t_elapsed = t_end - t_start;
    speed   = (float)QSPI_W25Q64_TEST_SIZE / 1024 / 1024 / t_elapsed * 1000;

    if (status == QSPI_W25Q64_OK) {
        DEBUG_PRINT("\r\n读取成功,数据大小：%lu KB, 耗时: %lu ms, 读取速度：%.2f MB/s \r\n",
               QSPI_W25Q64_TEST_SIZE / 1024, t_elapsed, speed);
    } else {
        DEBUG_PRINT("\r\n读取错误!!!!!  错误代码:%d\r\n", status);
        return;
    }

    // ---- 数据校验 ----
    for (i = 0; i < QSPI_W25Q64_TEST_SIZE; i++) {
        if (write_buf[i] != read_buf[i]) {
            DEBUG_PRINT("\r\n数据校验失败!!!!!\r\n");
            return;
        }
    }
    DEBUG_PRINT("\r\n校验通过!!!!! QSPI驱动W25Q64测试正常\r\n");

    // ---- 整片 Flash 读取测速 ----
    DEBUG_PRINT("\r\n*****************************************************************************************************\r\n");
    DEBUG_PRINT("\r\n上面的测试中，读取的数据比较小，耗时很短，加之测量的最小单位为ms，计算出的读取速度误差较大\r\n");
    DEBUG_PRINT("\r\n接下来读取整片flash的数据用以测试速度，这样得出的速度误差比较小\r\n");
    DEBUG_PRINT("\r\n开始读取>>>>\r\n");

    test_addr = 0;
    t_start   = HAL_GetTick();

    for (i = 0; i < QSPI_W25Q64_FLASH_SIZE / QSPI_W25Q64_TEST_SIZE; i++) {
        memcpy(read_buf, (uint8_t *)QSPI_W25Q64_MEM_ADDR + test_addr, QSPI_W25Q64_TEST_SIZE);
        test_addr += QSPI_W25Q64_TEST_SIZE;
    }
    t_end     = HAL_GetTick();
    t_elapsed = t_end - t_start;
    speed     = (float)QSPI_W25Q64_FLASH_SIZE / 1024 / 1024 / t_elapsed * 1000;

    DEBUG_PRINT("\r\n读取成功,数据大小：%lu MB, 耗时: %lu ms, 读取速度：%.2f MB/s \r\n",
           QSPI_W25Q64_FLASH_SIZE / 1024 / 1024, t_elapsed, speed);
}

#endif // USE_BSP_UNIT_TESTING
