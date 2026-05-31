#ifndef BSP_TUSB_PORT_H_
#define BSP_TUSB_PORT_H_

#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// TinyUSB 移植公共接口
//--------------------------------------------------------------------+

int  tusb_port_init(void);      // 初始化 TinyUSB 设备栈
void tusb_port_task(void);      // TinyUSB 设备任务 (需在主循环中轮询)

#ifdef __cplusplus
}
#endif

#endif
