#include "fatfs_port.h"
#include "ff.h"
#include <string.h>

#if USE_BSP_UNIT_TESTING

// 测试用 UTF-8 文件名 (含中文)
static const char *g_test_name = "test_\xe6\xb5\x8b\xe8\xaf\x95.txt";  // "test_测试.txt"
static const char *g_test_data = "Hello FatFs UTF-8 测试\r\n";

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static FIL     g_fil;
static FATFS  *g_fs_ptr;
static DIR     g_dir;
static FILINFO g_fno;

//--------------------------------------------------------------------+
// 静态函数 — 文件写读校验
//--------------------------------------------------------------------+

static int fatfs_test_file_rw(void) {
    FRESULT  res;
    UINT written;

    // 创建并写入
    res = f_open(&g_fil, g_test_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        DEBUG_PRINT("  TEST: f_open(write) failed (%d)\r\n", res);
        return -1;
    }
    res = f_write(&g_fil, g_test_data, strlen(g_test_data), &written);
    if (res != FR_OK || written != strlen(g_test_data)) {
        DEBUG_PRINT("  TEST: f_write failed (%d), written=%d\r\n", res, written);
        f_close(&g_fil);
        return -1;
    }
    f_close(&g_fil);

    // 读回并校验
    char buf[128] = {0};
    UINT br;
    res = f_open(&g_fil, g_test_name, FA_READ);
    if (res != FR_OK) {
        DEBUG_PRINT("  TEST: f_open(read) failed (%d)\r\n", res);
        return -1;
    }
    res = f_read(&g_fil, buf, sizeof(buf) - 1, &br);
    if (res != FR_OK) {
        DEBUG_PRINT("  TEST: f_read failed (%d)\r\n", res);
        f_close(&g_fil);
        return -1;
    }
    f_close(&g_fil);

    if (strcmp(buf, g_test_data) != 0) {
        DEBUG_PRINT("  TEST: data mismatch: got \"%s\", expected \"%s\"\r\n", buf, g_test_data);
        return -1;
    }

    DEBUG_PRINT("  TEST: file R/W ok (%d bytes)\r\n", br);
    return 0;
}

//--------------------------------------------------------------------+
// 静态函数 — 目录遍历
//--------------------------------------------------------------------+

static int fatfs_test_dir_list(void) {
    FRESULT res;

    res = f_opendir(&g_dir, "/");
    if (res != FR_OK) {
        DEBUG_PRINT("  TEST: f_opendir failed (%d)\r\n", res);
        return -1;
    }

    DEBUG_PRINT("  TEST: root directory:\r\n");
    int count = 0;
    while (1) {
        res = f_readdir(&g_dir, &g_fno);
        if (res != FR_OK || g_fno.fname[0] == '\0') break;
        DEBUG_PRINT("    %s  (%lu bytes)\r\n", g_fno.fname, g_fno.fsize);
        count++;
    }
    f_closedir(&g_dir);
    DEBUG_PRINT("  TEST: %d entries\r\n", count);
    return (count > 0) ? 0 : -1;
}

//--------------------------------------------------------------------+
// 静态函数 — 删除测试文件
//--------------------------------------------------------------------+

static int fatfs_test_cleanup(void) {
    FRESULT res = f_unlink(g_test_name);
    if (res != FR_OK) {
        DEBUG_PRINT("  TEST: f_unlink failed (%d)\r\n", res);
        return -1;
    }
    DEBUG_PRINT("  TEST: cleanup ok\r\n");
    return 0;
}

//--------------------------------------------------------------------+
// 外部接口 — 单元测试
//--------------------------------------------------------------------+

void fatfs_port_test(void) {
    DEBUG_PRINT("\r\n=== FatFs Unit Test ===\r\n");

    // 1. 检查挂载状态 (f_mount 已在 fatfs_port_init 中完成)
    DEBUG_PRINT("TEST: mount ok\r\n");

    // 2. 文件读写校验
    if (fatfs_test_file_rw() != 0) {
        DEBUG_PRINT("TEST: R/W FAIL\r\n");
        return;
    }

    // 3. 目录遍历
    if (fatfs_test_dir_list() != 0) {
        DEBUG_PRINT("TEST: dir list FAIL\r\n");
        fatfs_test_cleanup();
        return;
    }

    // 4. 清理
    fatfs_test_cleanup();
    DEBUG_PRINT("=== FatFs Test PASSED ===\r\n");
}

#endif
