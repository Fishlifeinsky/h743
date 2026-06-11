#include "ctpb_frame.h"
#include "protobuf-c.h"

#include <string.h>
#include <stdlib.h>

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "message_buffer.h"
#else
#include "elib/ring_buffer.h"
#endif

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

// 帧解码状态
typedef enum {
    FRAME_SYNC0 = 0,  // 等待 0xFF
    FRAME_SYNC1,      // 等待 'V'
    FRAME_SYNC2,      // 等待 'B'
    FRAME_LEN_LO,     // 读取长度低字节
    FRAME_LEN_HI,     // 读取长度高字节
    FRAME_DATA,       // 读取 payload
} frame_state_t;

// 输入环形缓冲区
#if BSP_FREERTOS_ENABLED
static StreamBufferHandle_t  g_stream;
static uint8_t               g_stream_storage[CTPB_RING_SIZE + 1];
static StaticStreamBuffer_t  g_stream_struct;
#else
static uint8_t    g_ring_storage[CTPB_RING_SIZE];
static ring_buf_t g_ring;
#endif

// 帧解码
static frame_state_t g_state = FRAME_SYNC0;
static uint8_t       g_buf[CTPB_FRAME_BUF_SIZE];  // 当前帧 payload
static uint16_t      g_len;
static uint16_t      g_pos;

// 输出: FreeRTOS 消息缓冲区 (解码后的完整帧)
#if BSP_FREERTOS_ENABLED
static MessageBufferHandle_t  g_msg_buf;
static uint8_t                g_msg_storage[1024];
static StaticMessageBuffer_t  g_msg_struct;
#endif

//--------------------------------------------------------------------+
// 外部接口 — 初始化
//--------------------------------------------------------------------+

void ctpb_frame_init(void) {
#if BSP_FREERTOS_ENABLED
    g_stream = xStreamBufferCreateStatic(
        sizeof(g_stream_storage),
        1,  // trigger level (不阻塞接收，设为1)
        g_stream_storage,
        &g_stream_struct
    );
#else
    ring_buf_init(&g_ring, g_ring_storage, sizeof(g_ring_storage));
#endif
    g_state = FRAME_SYNC0;
    g_len   = 0;
    g_pos   = 0;
}

void ctpb_frame_rtos_init(void) {
#if BSP_FREERTOS_ENABLED
    g_msg_buf = xMessageBufferCreateStatic(
        sizeof(g_msg_storage),
        g_msg_storage,
        &g_msg_struct
    );
#endif
}

//--------------------------------------------------------------------+
// 外部接口 — 打包
//--------------------------------------------------------------------+

size_t ctpb_frame_pack(const ProtobufCMessage *msg, uint8_t **out) {
    size_t pb_len = protobuf_c_message_get_packed_size(msg);
    if (pb_len == 0 || pb_len > CTPB_FRAME_BUF_SIZE) {
        *out = NULL;
        return 0;
    }

    // 帧: \xFF V B + len(2B LE) + payload
    size_t frame_len = 3 + 2 + pb_len;
    *out = (uint8_t *)malloc(frame_len);
    if (*out == NULL) return 0;

    (*out)[0] = 0xFF;
    (*out)[1] = 'V';
    (*out)[2] = 'B';
    (*out)[3] = (uint8_t)(pb_len & 0xFF);
    (*out)[4] = (uint8_t)((pb_len >> 8) & 0xFF);

    // 直接 pack 到帧内偏移, 避免中间 buffer
    size_t packed = protobuf_c_message_pack(msg, *out + 5);
    if (packed != pb_len) {
        free(*out);
        *out = NULL;
        return 0;
    }
    return frame_len;
}

//--------------------------------------------------------------------+
// 帧解码状态机
//--------------------------------------------------------------------+

// 完整帧推入 FreeRTOS 消息缓冲区
static void push_frame(const uint8_t *data, size_t len) {
#if BSP_FREERTOS_ENABLED
    if (g_msg_buf) {
        xMessageBufferSend(g_msg_buf, data, len, 0);
    }
#else
    (void)data;
    (void)len;
#endif
}

// 从环形缓冲区读取一个字节, 返回 false 表示缓冲区空
static bool input_get(uint8_t *byte) {
#if BSP_FREERTOS_ENABLED
    return xStreamBufferReceive(g_stream, byte, 1, 0) == 1;
#else
    return ring_buf_get(&g_ring, byte);
#endif
}

// 写入环形缓冲区
static size_t input_write(const uint8_t *data, size_t len) {
#if BSP_FREERTOS_ENABLED
    return xStreamBufferSend(g_stream, data, len, 0);
#else
    return ring_buf_write(&g_ring, data, len);
#endif
}

// 推进状态机一步
// 返回 true 表示继续推进, false 表示缓冲区空
static bool step(void) {
    uint8_t byte;
    if (!input_get(&byte)) return false;

    switch (g_state) {
    case FRAME_SYNC0:
        if (byte == 0xFF) g_state = FRAME_SYNC1;
        break;

    case FRAME_SYNC1:
        if (byte == 'V') {
            g_state = FRAME_SYNC2;
        } else {
            g_state = FRAME_SYNC0;
            if (byte == 0xFF) g_state = FRAME_SYNC1;
        }
        break;

    case FRAME_SYNC2:
        if (byte == 'B') {
            g_state = FRAME_LEN_LO;
        } else {
            g_state = FRAME_SYNC0;
            if (byte == 0xFF) g_state = FRAME_SYNC1;
        }
        break;

    case FRAME_LEN_LO:
        g_len   = byte;
        g_state = FRAME_LEN_HI;
        break;

    case FRAME_LEN_HI:
        g_len |= (uint16_t)byte << 8;
        if (g_len > CTPB_FRAME_BUF_SIZE || g_len == 0) {
            g_state = FRAME_SYNC0;  // 无效长度, 丢弃并重新同步
        } else {
            g_pos   = 0;
            g_state = FRAME_DATA;
        }
        break;

    case FRAME_DATA:
        g_buf[g_pos++] = byte;
        if (g_pos >= g_len) {
            push_frame(g_buf, g_len);
            g_state = FRAME_SYNC0;
        }
        break;
    }
    return true;
}

//--------------------------------------------------------------------+
// 外部接口 — 喂入 / 接收
//--------------------------------------------------------------------+

void ctpb_frame_feed(const uint8_t *data, size_t len) {
    // 写入环形缓冲区 (满时丢弃, 不阻塞)
    input_write(data, len);

    // 尽可能推进状态机
    while (step()) {}
}

size_t ctpb_frame_recv(uint8_t *buf, size_t max_len, uint32_t timeout_ms) {
#if BSP_FREERTOS_ENABLED
    if (!g_msg_buf) return 0;
    return xMessageBufferReceive(g_msg_buf, buf, max_len,
                                 pdMS_TO_TICKS(timeout_ms));
#else
    (void)buf;
    (void)max_len;
    (void)timeout_ms;
    return 0;
#endif
}
