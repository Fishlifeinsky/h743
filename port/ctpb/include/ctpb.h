#ifndef CTPB_H_
#define CTPB_H_

#include "protobuf-c.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void ctpb_init(void);       // 初始化 protobuf-c + 帧层
void ctpb_rtos_init(void);  // FreeRTOS 资源初始化 (调度器启动后调用)
void ctpb_poll(void);       // 轮询接收并分发帧

// 注册传输层发送回调 (供 ctpb 内部发送响应帧)
typedef void (*ctpb_send_cb_t)(const uint8_t *data, size_t len);
void ctpb_set_send_cb(ctpb_send_cb_t cb);

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void ctpb_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
