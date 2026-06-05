#include "vfs.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include "port/config.h"

// 标准库 O_* flags → VFS flags
int vfs_from_std_flags(int std_flags) {
    int v = 0;
    int acc = std_flags & O_ACCMODE;
    if (acc == O_RDONLY)   v |= VFS_O_RDONLY;
    if (acc == O_WRONLY)   v |= VFS_O_WRONLY;
    if (acc == O_RDWR)     v |= VFS_O_RDWR;
    if (std_flags & O_CREAT)   v |= VFS_O_CREAT;
    if (std_flags & O_EXCL)    v |= VFS_O_EXCL;
    if (std_flags & O_TRUNC)   v |= VFS_O_TRUNC;
    if (std_flags & O_APPEND)  v |= VFS_O_APPEND;
    if (std_flags & O_NONBLOCK) v |= VFS_O_NONBLOCK;
    return v;
}

// 全局 VFS 节点链表
static elist_t g_vfs_list;

// 去除 root 尾部 '/' 后的长度
static size_t root_effective_len(const char *root) {
    size_t len = strlen(root);
    while (len > 1 && root[len - 1] == '/') {
        len--;
    }
    return len;
}

// 匹配 path 到 vfs_node，返回最长匹配的节点及剩余路径
// 剩余路径写入 remainder (由调用者提供缓冲区)
static vfs_node_t *vfs_match(const char *path, const char **remainder) {
    vfs_node_t *best = NULL;
    size_t      best_len = 0;
    const char *best_rem = NULL;

    elist_node_t *cur = g_vfs_list.head;
    while (cur) {
        vfs_node_t *node = (vfs_node_t *)cur->data;
        size_t rlen = root_effective_len(node->root);

        // path 必须以 node->root 的有效部分开头
        if (strncmp(path, node->root, rlen) != 0) {
            cur = cur->next;
            continue;
        }

        // path[rlen] 必须是 '\0' 或 '/'
        if (path[rlen] != '\0' && path[rlen] != '/') {
            cur = cur->next;
            continue;
        }

        // 取最长匹配
        if (rlen > best_len) {
            best_len = rlen;
            best     = node;
            if (path[rlen] == '/') {
                best_rem = path + rlen + 1;
            } else {
                best_rem = "";
            }
        }

        cur = cur->next;
    }

    if (remainder) {
        *remainder = best_rem;
    }
    return best;
}

//---- 节点生命周期 ----

vfs_node_t *vfs_create_node(void) {
    vfs_node_t *node = (vfs_node_t *)malloc(sizeof(vfs_node_t));
    if (node) {
        memset(node, 0, sizeof(vfs_node_t));
    }
    return node;
}

void vfs_install_node(vfs_node_t *node) {
    if (!node) return;

    if (elist_empty(&g_vfs_list)) {
        elist_init(&g_vfs_list);
    }
    elist_push_back(&g_vfs_list, node);
}

void vfs_uninstall_node(vfs_node_t *node) {
    if (!node) return;
    elist_remove_data(&g_vfs_list, node);
}

//---- VFS 文件操作 ----

vfs_handle_t *vfs_open(const char *path, int flags) {
    DEBUG_PRINT("vfs_open: %s flags=%d\n", path, flags);
    const char *remainder = NULL;
    vfs_node_t *node = vfs_match(path, &remainder);
    if (!node) {
        DEBUG_PRINT("vfs_open: no match for %s\n", path);
        return NULL;
    }
    DEBUG_PRINT("vfs_open: match %s, remainder=\"%s\"\n", node->root, remainder);

    vfs_handle_t *handle = (vfs_handle_t *)malloc(sizeof(vfs_handle_t));
    if (!handle) {
        DEBUG_PRINT("vfs_open: malloc handle failed\n");
        return NULL;
    }

    handle->node = node;

    if (node->open) {
        handle->node_handle = node->open(remainder, flags);
    } else {
        handle->node_handle = NULL;
    }

    node->opened = true;
    DEBUG_PRINT("vfs_open: ok handle=%p\n", handle);
    return handle;
}

int vfs_close(vfs_handle_t *handle) {
    if (!handle) return -1;

    int ret = 0;
    if (handle->node && handle->node->close) {
        ret = handle->node->close(handle->node_handle);
    }

    if (handle->node) {
        handle->node->opened = false;
    }

    free(handle);
    return ret;
}

int vfs_read(vfs_handle_t *handle, void *buf, size_t len) {
    if (!handle || !handle->node || !handle->node->read) return -1;
    return handle->node->read(handle->node_handle, buf, len);
}

int vfs_write(vfs_handle_t *handle, const void *buf, size_t len) {
    if (!handle || !handle->node || !handle->node->write) return -1;
    return handle->node->write(handle->node_handle, buf, len);
}

int vfs_lseek(vfs_handle_t *handle, int offset, int whence) {
    if (!handle || !handle->node || !handle->node->lseek) return -1;
    return handle->node->lseek(handle->node_handle, offset, whence);
}

int vfs_link(const char *oldpath, const char *newpath) {
    const char *rem_old = NULL;
    const char *rem_new = NULL;
    vfs_node_t *old_node = vfs_match(oldpath, &rem_old);
    vfs_node_t *new_node = vfs_match(newpath, &rem_new);

    // 必须在同一个节点下
    if (!old_node || old_node != new_node) return -1;
    if (!old_node->link) return -1;

    return old_node->link(rem_old, rem_new);
}

int vfs_unlink(const char *path) {
    const char *remainder = NULL;
    vfs_node_t *node = vfs_match(path, &remainder);
    if (!node || !node->unlink) return -1;
    return node->unlink(remainder);
}

int vfs_stat(const char *path, struct stat *st) {
    const char *remainder = NULL;
    vfs_node_t *node = vfs_match(path, &remainder);
    if (!node || !node->stat) return -1;
    return node->stat(remainder, st);
}

int vfs_fstat(vfs_handle_t *handle, struct stat *st) {
    if (!handle || !handle->node || !handle->node->fstat) return -1;
    return handle->node->fstat(handle->node_handle, st);
}
