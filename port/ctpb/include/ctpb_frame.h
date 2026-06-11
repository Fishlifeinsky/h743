#ifndef CTPB_FRAME_H_
#define CTPB_FRAME_H_

#include <stdint.h>
#include <stddef.h>
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 帧格式: \xFF V B + len(2B LE) + payload
// 接收环形缓冲区大小 (raw bytes, 含帧头开销)
//--------------------------------------------------------------------+

#define CTPB_FRAME_BUF_SIZE   256   // payload 上限
#define CTPB_RING_SIZE        (CTPB_FRAME_BUF_SIZE * 2)  // 输入环形缓冲区

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

// 初始化帧状态机
void ctpb_frame_init(void);

// FreeRTOS 消息缓冲区初始化 (调度器启动后调用)
void ctpb_frame_rtos_init(void);

// 打包: ProtobufCMessage -> 帧字节流 (malloc 分配, 调用者释放)
// 返回帧总长度, 失败返回 0
size_t ctpb_frame_pack(const ProtobufCMessage *msg, uint8_t **out);

// 喂入原始字节 (写入环形缓冲区), 自动推进状态机
void ctpb_frame_feed(const uint8_t *data, size_t len);

// 从消息队列取出一个解码帧 (payload only)
// 返回 payload 长度, 0 表示超时无数据
size_t ctpb_frame_recv(uint8_t *buf, size_t max_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
