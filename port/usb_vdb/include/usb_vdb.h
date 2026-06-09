#ifndef USB_VDB_H
#define USB_VDB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 公开 API
//--------------------------------------------------------------------+

void usb_vdb_init(void);
void usb_vdb_poll(void);

#ifdef __cplusplus
}
#endif

#endif
