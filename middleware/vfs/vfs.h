#ifndef VFS_H_
#define VFS_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// VFS — 虚拟文件系统
//
// 每个 vfs_node 代表一个挂载点, 挂载在根 "/" 下的某个目录前缀上.
// vfs_open 时按最长前缀匹配找到对应节点, 调其 open 回调.
//
// 示例:
//   节点 root="/dev"   → vfs_open("/dev/adb") 匹配, 调 open("adb")
//   节点 root="/dev/adb" → vfs_open("/dev/adb") 匹配, 调 open("")
//   节点 root="/dev/adbc" → vfs_open("/dev/adb") 不匹配
//   节点 root="/dev/adb" → vfs_open("/dev")     不匹配
//--------------------------------------------------------------------+

// 可替换的依赖头文件

#include "elib/list.h"
#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

//--------------------------------------------------------------------+
// 文件类型
//--------------------------------------------------------------------+

typedef enum {
    VFS_TYPE_DIR    = 0,  // 目录
    VFS_TYPE_FILE   = 1,  // 文件
    VFS_TYPE_DEVICE = 2,  // 设备
    VFS_TYPE_LINK   = 3,  // 链接
} vfs_type_t;

//--------------------------------------------------------------------+
// open 标志位 (自定义, 与标准库 O_* 无关)
// 通过 vfs_from_std_flags() 转换
//--------------------------------------------------------------------+

#define VFS_O_RDONLY    0x0001
#define VFS_O_WRONLY    0x0002
#define VFS_O_RDWR      0x0004
#define VFS_O_CREAT     0x0008
#define VFS_O_EXCL      0x0010
#define VFS_O_TRUNC     0x0020
#define VFS_O_APPEND    0x0040
#define VFS_O_NONBLOCK  0x0080
#define VFS_O_DIRECTORY 0x0100

// 将标准库 O_* flags 转换为 VFS flags
int vfs_from_std_flags(int std_flags);

//--------------------------------------------------------------------+
// lseek whence
//--------------------------------------------------------------------+

#define VFS_SEEK_SET  0
#define VFS_SEEK_CUR  1
#define VFS_SEEK_END  2

//--------------------------------------------------------------------+
// VFS 节点 (一个挂载点)
//--------------------------------------------------------------------+

typedef struct vfs_node {
    elist_node_t node;             // 链表节点
    char         root[32];         // 根目录前缀 (如 "/dev/adb")
    vfs_type_t   type;             // 文件类型
    bool         opened;           // 是否已打开

    // 文件操作回调 (由实现者填充)
    void *(*open)  (const char *path, int flags);
    int   (*close) (void *handle);
    int   (*read)  (void *handle, void *buf, size_t len);
    int   (*write) (void *handle, const void *buf, size_t len);
    int   (*lseek) (void *handle, int offset, int whence);
    int   (*link)  (const char *oldpath, const char *newpath);
    int   (*unlink)(const char *path);
    int   (*stat)  (const char *path, struct stat *st);
    int   (*fstat) (void *handle, struct stat *st);

    void *user_data;               // 用户自定义数据
} vfs_node_t;

//--------------------------------------------------------------------+
// VFS 文件句柄
//--------------------------------------------------------------------+

typedef struct vfs_handle {
    vfs_node_t *node;
    void       *node_handle;       // 由 node->open 返回
} vfs_handle_t;

//--------------------------------------------------------------------+
// 节点生命周期
//--------------------------------------------------------------------+

vfs_node_t *vfs_create_node(void);
void        vfs_install_node(vfs_node_t *node);
void        vfs_uninstall_node(vfs_node_t *node);

//--------------------------------------------------------------------+
// VFS 文件操作
//--------------------------------------------------------------------+

vfs_handle_t *vfs_open  (const char *path, int flags);
int           vfs_close (vfs_handle_t *handle);
int           vfs_read  (vfs_handle_t *handle, void *buf, size_t len);
int           vfs_write (vfs_handle_t *handle, const void *buf, size_t len);
int           vfs_lseek (vfs_handle_t *handle, int offset, int whence);
int           vfs_link  (const char *oldpath, const char *newpath);
int           vfs_unlink(const char *path);
int           vfs_stat  (const char *path, struct stat *st);
int           vfs_fstat (vfs_handle_t *handle, struct stat *st);

#ifdef __cplusplus
}
#endif

#endif
