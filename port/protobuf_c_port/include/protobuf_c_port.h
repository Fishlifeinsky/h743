#ifndef BSP_PROTOBUF_C_PORT_H_
#define BSP_PROTOBUF_C_PORT_H_

#include "protobuf-c.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

// 打包消息到动态分配的 buffer，调用者负责释放
// 返回打包后的字节数，失败返回 0
size_t protobuf_c_port_pack(ProtobufCMessage *msg, uint8_t **out);

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void protobuf_c_port_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
