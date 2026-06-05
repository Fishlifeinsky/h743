#include "fatfs_port.h"
#include "ff.h"
#include "vfs.h"
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

//--------------------------------------------------------------------+
// VFS flags → FatFs flags
//--------------------------------------------------------------------+

static BYTE vfs_to_fatfs_flags(int vfs_flags) {
    BYTE fa = 0;

    if (vfs_flags & VFS_O_RDWR) {
        fa |= FA_READ | FA_WRITE;
    } else if (vfs_flags & VFS_O_WRONLY) {
        fa |= FA_WRITE;
    } else {
        fa |= FA_READ;
    }

    if (vfs_flags & VFS_O_CREAT) {
        if (vfs_flags & VFS_O_EXCL) {
            fa |= FA_CREATE_NEW;
        } else if (vfs_flags & VFS_O_TRUNC) {
            fa |= FA_CREATE_ALWAYS;
        } else {
            fa |= FA_OPEN_ALWAYS;
        }
    }

    if (vfs_flags & VFS_O_APPEND) {
        fa |= FA_OPEN_APPEND;
    }

    return fa;
}

//--------------------------------------------------------------------+
// FILINFO → struct stat
//--------------------------------------------------------------------+

static void filinfo_to_stat(const FILINFO *fno, struct stat *st) {
    memset(st, 0, sizeof(*st));
    st->st_mode = (fno->fattrib & AM_DIR) ? S_IFDIR : S_IFREG;
    st->st_size = fno->fsize;
}

//--------------------------------------------------------------------+
// VFS 回调 — 封装 FatFs
//--------------------------------------------------------------------+

static void *fatfs_vfs_open(const char *path, int flags) {
    DEBUG_PRINT("  VFS: fatfs_open path=\"%s\" flags=0x%X\r\n", path, flags);

    FIL *fp = malloc(sizeof(FIL));
    if (!fp) {
        DEBUG_PRINT("  VFS: fatfs_open malloc FIL failed\r\n");
        return NULL;
    }

    BYTE fa = vfs_to_fatfs_flags(flags);
    DEBUG_PRINT("  VFS: f_open fa=0x%X\r\n", fa);
    FRESULT res = f_open(fp, path, fa);
    if (res != FR_OK) {
        DEBUG_PRINT("  VFS: f_open failed (%d)\r\n", res);
        free(fp);
        return NULL;
    }

    DEBUG_PRINT("  VFS: fatfs_open ok fp=%p\r\n", fp);
    return fp;
}

static int fatfs_vfs_close(void *handle) {
    FIL *fp = (FIL *)handle;
    f_close(fp);
    free(fp);
    return 0;
}

static int fatfs_vfs_read(void *handle, void *buf, size_t len) {
    FIL *fp = (FIL *)handle;
    UINT br;
    FRESULT res = f_read(fp, buf, (UINT)len, &br);
    if (res != FR_OK) return -1;
    return (int)br;
}

static int fatfs_vfs_write(void *handle, const void *buf, size_t len) {
    FIL *fp = (FIL *)handle;
    UINT bw;
    FRESULT res = f_write(fp, buf, (UINT)len, &bw);
    if (res != FR_OK) return -1;
    return (int)bw;
}

static int fatfs_vfs_lseek(void *handle, int offset, int whence) {
    FIL *fp = (FIL *)handle;
    FSIZE_t pos;

    switch (whence) {
    case VFS_SEEK_SET: pos = (FSIZE_t)offset;                      break;
    case VFS_SEEK_CUR: pos = f_tell(fp) + (FSIZE_t)offset;         break;
    case VFS_SEEK_END: pos = f_size(fp) + (FSIZE_t)offset;         break;
    default:           return -1;
    }

    FRESULT res = f_lseek(fp, pos);
    return (res == FR_OK) ? (int)f_tell(fp) : -1;
}

static int fatfs_vfs_stat(const char *path, struct stat *st) {
    FILINFO fno;
    FRESULT res = f_stat(path, &fno);
    if (res != FR_OK) return -1;
    filinfo_to_stat(&fno, st);
    return 0;
}

static int fatfs_vfs_fstat(void *handle, struct stat *st) {
    FIL *fp = (FIL *)handle;
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG;
    st->st_size = f_size(fp);
    return 0;
}

static int fatfs_vfs_unlink(const char *path) {
    return (f_unlink(path) == FR_OK) ? 0 : -1;
}

static int fatfs_vfs_link(const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    return -1;
}

//--------------------------------------------------------------------+
// 注册 VFS 节点
//--------------------------------------------------------------------+

void fatfs_port_vfs_init(void) {
    vfs_node_t *node = vfs_create_node();
    if (!node) {
        DEBUG_PRINT("  VFS: create_node failed\r\n");
        return;
    }

    strncpy(node->root, "/sd", sizeof(node->root) - 1);
    node->type = VFS_TYPE_DIR;

    node->open   = fatfs_vfs_open;
    node->close  = fatfs_vfs_close;
    node->read   = fatfs_vfs_read;
    node->write  = fatfs_vfs_write;
    node->lseek  = fatfs_vfs_lseek;
    node->link   = fatfs_vfs_link;
    node->unlink = fatfs_vfs_unlink;
    node->stat   = fatfs_vfs_stat;
    node->fstat  = fatfs_vfs_fstat;

    vfs_install_node(node);
    DEBUG_PRINT("VFS: node /sd installed\r\n");
}
