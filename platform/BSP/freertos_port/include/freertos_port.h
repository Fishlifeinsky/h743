#ifndef BSP_FREERTOS_PORT_H_
#define BSP_FREERTOS_PORT_H_

#include "BSP/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if BSP_FREERTOS_ENABLED

#include "FreeRTOS.h"
#include "task.h"

void freertos_port_init(void);

#else
#define freertos_port_init() ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
