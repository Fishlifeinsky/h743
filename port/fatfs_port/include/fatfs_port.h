#ifndef FATFS_PORT_H_
#define FATFS_PORT_H_

#include <stdint.h>
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// FatFs 移植公共接口
//--------------------------------------------------------------------+

int  fatfs_port_init(void);       // 挂载 SD 卡文件系统 (f_mount)
void fatfs_port_vfs_init(void);   // 注册 VFS 节点 (根 "/sd")
void fatfs_port_get_info(void);   // 打印 SD 卡信息

#if USE_BSP_UNIT_TESTING
void fatfs_port_test(void);
void fatfs_port_vfs_test(void);
#endif

//--------------------------------------------------------------------+
// SD 卡底层操作 (供 3rd/fatfs/diskio.c 调用)
//--------------------------------------------------------------------+

int fatfs_port_sd_init(void);     // 初始化 SD 卡, 0=成功
int fatfs_port_sd_status(void);   // 获取 SD 卡状态, 0=就绪
int fatfs_port_sd_read(uint8_t *buf, uint32_t sector, uint32_t count);
int fatfs_port_sd_write(const uint8_t *buf, uint32_t sector, uint32_t count);
int fatfs_port_sd_ioctl(uint8_t cmd, void *buf);  // 0=成功, -1=不支持

#ifdef __cplusplus
}
#endif

#endif
