#include "fatfs_port.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#if USE_BSP_UNIT_TESTING

static const char *g_test_path = "/sd/vfs_test.txt";
static const char *g_test_data = "Hello VFS stdio test!";

// ---- POSIX 级 I/O 测试 (open/read/write/close/lseek) ----

static int vfs_test_posix_rw(void) {
    int fd = open(g_test_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        DEBUG_PRINT("  VFS TEST: open(w) failed (errno=%d)\r\n", errno);
        return -1;
    }
    DEBUG_PRINT("  VFS TEST: open(w) ok fd=%d\r\n", fd);

    int n = write(fd, g_test_data, strlen(g_test_data));
    if (n != (int)strlen(g_test_data)) {
        DEBUG_PRINT("  VFS TEST: write failed (%d/%u)\r\n", n, (unsigned)strlen(g_test_data));
        close(fd);
        return -1;
    }
    close(fd);

    fd = open(g_test_path, O_RDONLY);
    if (fd < 0) {
        DEBUG_PRINT("  VFS TEST: open(r) failed\r\n");
        return -1;
    }
    char buf[128] = {0};
    n = read(fd, buf, sizeof(buf) - 1);
    if (n != (int)strlen(g_test_data)) {
        DEBUG_PRINT("  VFS TEST: read failed (%d/%u)\r\n", n, (unsigned)strlen(g_test_data));
        close(fd);
        return -1;
    }
    close(fd);

    if (strcmp(buf, g_test_data) != 0) {
        DEBUG_PRINT("  VFS TEST: data mismatch\r\n");
        return -1;
    }

    DEBUG_PRINT("  VFS TEST: posix R/W ok\r\n");
    return 0;
}

static int vfs_test_posix_lseek(void) {
    int fd = open(g_test_path, O_RDONLY);
    if (fd < 0) return -1;

    int pos = lseek(fd, 0, SEEK_END);
    if (pos != (int)strlen(g_test_data)) {
        DEBUG_PRINT("  VFS TEST: lseek END: %d, expected %u\r\n", pos, (unsigned)strlen(g_test_data));
        close(fd);
        return -1;
    }
    pos = lseek(fd, 0, SEEK_SET);
    if (pos != 0) {
        DEBUG_PRINT("  VFS TEST: lseek SET != 0\r\n");
        close(fd);
        return -1;
    }
    close(fd);
    DEBUG_PRINT("  VFS TEST: posix lseek ok\r\n");
    return 0;
}

static int vfs_test_posix_stat(void) {
    struct stat st;
    if (stat(g_test_path, &st) != 0) {
        DEBUG_PRINT("  VFS TEST: stat failed\r\n");
        return -1;
    }
    if (st.st_size != (off_t)strlen(g_test_data)) {
        DEBUG_PRINT("  VFS TEST: stat size mismatch\r\n");
        return -1;
    }
    DEBUG_PRINT("  VFS TEST: posix stat ok (size=%ld)\r\n", (long)st.st_size);

    int fd = open(g_test_path, O_RDONLY);
    if (fd < 0) return -1;
    if (fstat(fd, &st) != 0) {
        DEBUG_PRINT("  VFS TEST: fstat failed\r\n");
        close(fd);
        return -1;
    }
    close(fd);
    DEBUG_PRINT("  VFS TEST: posix fstat ok\r\n");
    return 0;
}

// ---- cleanup ----

static int vfs_test_cleanup(void) {
    if (remove(g_test_path) != 0) {
        DEBUG_PRINT("  VFS TEST: remove failed\r\n");
        return -1;
    }
    DEBUG_PRINT("  VFS TEST: cleanup ok\r\n");
    return 0;
}

// ---- stdio 级测试 (fopen/fread/fwrite) ----

static int vfs_test_stdio_rw(void) {
    FILE *fp = fopen(g_test_path, "w");
    if (!fp) {
        DEBUG_PRINT("  VFS TEST: fopen(w) failed (errno=%d)\r\n", errno);
        return -1;
    }
    size_t n = fwrite(g_test_data, 1, strlen(g_test_data), fp);
    if (n != strlen(g_test_data)) {
        DEBUG_PRINT("  VFS TEST: fwrite failed\r\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    fp = fopen(g_test_path, "r");
    if (!fp) {
        DEBUG_PRINT("  VFS TEST: fopen(r) failed\r\n");
        return -1;
    }
    char buf[128] = {0};
    n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    if (n != strlen(g_test_data) || strcmp(buf, g_test_data) != 0) {
        DEBUG_PRINT("  VFS TEST: stdio read mismatch\r\n");
        return -1;
    }
    DEBUG_PRINT("  VFS TEST: stdio R/W ok\r\n");
    return 0;
}

//--------------------------------------------------------------------+
// 测试入口
//--------------------------------------------------------------------+

void fatfs_port_vfs_test(void) {
    DEBUG_PRINT("\r\n=== FatFs VFS Test (posix) ===\r\n");

    if (vfs_test_posix_rw() != 0) {
        DEBUG_PRINT("VFS TEST: posix R/W FAIL\r\n");
        return;
    }
    if (vfs_test_posix_lseek() != 0) {
        DEBUG_PRINT("VFS TEST: posix lseek FAIL\r\n");
        remove(g_test_path);
        return;
    }
    if (vfs_test_posix_stat() != 0) {
        DEBUG_PRINT("VFS TEST: posix stat FAIL\r\n");
        remove(g_test_path);
        return;
    }
    DEBUG_PRINT("=== FatFs VFS Posix Test PASSED ===\r\n");

    // 清理后重新建文件, 再测 stdio
    remove(g_test_path);
    DEBUG_PRINT("\r\n=== FatFs VFS Test (stdio) ===\r\n");
    if (vfs_test_stdio_rw() != 0) {
        DEBUG_PRINT("VFS TEST: stdio R/W FAIL\r\n");
        return;
    }

    if (vfs_test_cleanup() != 0) {
        DEBUG_PRINT("VFS TEST: cleanup FAIL\r\n");
        return;
    }
    DEBUG_PRINT("=== FatFs VFS Test PASSED ===\r\n");
}

#endif
