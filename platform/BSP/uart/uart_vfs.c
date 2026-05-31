#include "uart.h"
#include "vfs.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

//--------------------------------------------------------------------+
// VFS 句柄
//--------------------------------------------------------------------+

typedef struct {
    int uart_id;
    int flags;
} uart_vfs_handle_t;

//--------------------------------------------------------------------+
// VFS 回调: open  — 解析路径 "COM{N}" → 串口 ID
//--------------------------------------------------------------------+

static void *uart_vfs_open(const char *path, int flags)
{
    DEBUG_PRINT("  VFS: uart_open path=\"%s\"\r\n", path);

    if (strncmp(path, "COM", 3) != 0) {
        DEBUG_PRINT("  VFS: uart_open bad prefix\r\n");
        return NULL;
    }

    char *end;
    int id = (int)strtoul(path + 3, &end, 10);
    if (end == path + 3 || *end != '\0') {
        DEBUG_PRINT("  VFS: uart_open bad id in \"%s\"\r\n", path);
        return NULL;
    }

    if (!uart_is_ready(id)) {
        DEBUG_PRINT("  VFS: uart_open id=%d not ready\r\n", id);
        return NULL;
    }

    uart_vfs_handle_t *h = malloc(sizeof(uart_vfs_handle_t));
    if (!h) return NULL;

    h->uart_id = id;
    h->flags   = flags;
    DEBUG_PRINT("  VFS: uart_open ok id=%d flags=0x%x handle=%p\r\n", id, flags, h);
    return h;
}

//--------------------------------------------------------------------+
// VFS 回调: close
//--------------------------------------------------------------------+

static int uart_vfs_close(void *handle)
{
    uart_vfs_handle_t *h = (uart_vfs_handle_t *)handle;
    int id = h->uart_id;
    free(h);
    DEBUG_PRINT("  VFS: uart_close id=%d\r\n", id);
    return 0;
}

//--------------------------------------------------------------------+
// VFS 回调: read  — 阻塞读取 (timeout=1000ms)
//--------------------------------------------------------------------+

static int uart_vfs_read(void *handle, void *buf, size_t len)
{
    uart_vfs_handle_t *h = (uart_vfs_handle_t *)handle;
    if (!(h->flags & (VFS_O_RDONLY | VFS_O_RDWR))) return -1;
    return (int)uart_poll(h->uart_id, (uint8_t *)buf, len);
}

//--------------------------------------------------------------------+
// VFS 回调: write  — 发送到串口
//--------------------------------------------------------------------+

static int uart_vfs_write(void *handle, const void *buf, size_t len)
{
    uart_vfs_handle_t *h = (uart_vfs_handle_t *)handle;
    if (!(h->flags & (VFS_O_WRONLY | VFS_O_RDWR))) return -1;
    return (int)uart_write(h->uart_id, (const uint8_t *)buf, len);
}

//--------------------------------------------------------------------+
// VFS 回调: lseek  — 串口不支持定位
//--------------------------------------------------------------------+

static int uart_vfs_lseek(void *handle, int offset, int whence)
{
    (void)handle;
    (void)offset;
    (void)whence;
    return 0;
}

//--------------------------------------------------------------------+
// VFS 回调: stat  — 设备节点
//--------------------------------------------------------------------+

static int uart_vfs_stat(const char *path, struct stat *st)
{
    if (!st) return -1;
    if (strncmp(path, "COM", 3) != 0) return -1;

    char *end;
    int id = (int)strtoul(path + 3, &end, 10);
    if (end == path + 3 || *end != '\0') return -1;
    if (!uart_is_ready(id)) return -1;

    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

//--------------------------------------------------------------------+
// VFS 回调: fstat  — 设备节点
//--------------------------------------------------------------------+

static int uart_vfs_fstat(void *handle, struct stat *st)
{
    (void)handle;
    if (!st) return -1;
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

//--------------------------------------------------------------------+
// VFS 回调: link / unlink  — 不支持
//--------------------------------------------------------------------+

static int uart_vfs_link(const char *oldpath, const char *newpath)
{
    (void)oldpath;
    (void)newpath;
    return -1;
}

static int uart_vfs_unlink(const char *path)
{
    (void)path;
    return -1;
}

//--------------------------------------------------------------------+
// 注册 VFS 节点 "/dev/uart"
//--------------------------------------------------------------------+

void uart_vfs_init(void)
{
    vfs_node_t *node = vfs_create_node();
    if (!node) {
        DEBUG_PRINT("  VFS: uart create_node failed\r\n");
        return;
    }

    strncpy(node->root, "/dev/uart", sizeof(node->root) - 1);
    node->type = VFS_TYPE_DEVICE;

    node->open   = uart_vfs_open;
    node->close  = uart_vfs_close;
    node->read   = uart_vfs_read;
    node->write  = uart_vfs_write;
    node->lseek  = uart_vfs_lseek;
    node->link   = uart_vfs_link;
    node->unlink = uart_vfs_unlink;
    node->stat   = uart_vfs_stat;
    node->fstat  = uart_vfs_fstat;

    vfs_install_node(node);
    DEBUG_PRINT("VFS: node /dev/uart installed\r\n");
}
