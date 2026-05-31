#ifndef BSP_USB_MSC_H_
#define BSP_USB_MSC_H_

#include <stdint.h>
#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// USB MSC 公共接口
//--------------------------------------------------------------------+

void usb_msc_init(void);    // 初始化 MSC (缓存 SD 卡信息)

#if USE_BSP_UNIT_TESTING
void usb_msc_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
