/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs          (C)ChaN, 2025             */
/*-----------------------------------------------------------------------*/
/* Glue layer: disk_* → fatfs_port_sd_*                                  */
/* Hardware implementation is in BSP/fatfs_port/fatfs_port.c             */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "fatfs_port.h"

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    (void)pdrv;
    if (fatfs_port_sd_init() != 0) return STA_NOINIT;
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
    (void)pdrv;
    if (fatfs_port_sd_status() != 0) return STA_NODISK;
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (fatfs_port_sd_read(buff, sector, count) != 0) return RES_ERROR;
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    (void)pdrv;
    if (fatfs_port_sd_write(buff, sector, count) != 0) return RES_ERROR;
    return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    (void)pdrv;
    if (fatfs_port_sd_ioctl(cmd, buff) != 0) return RES_PARERR;
    return RES_OK;
}
