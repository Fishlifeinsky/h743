#ifndef ELIB_RING_BUF_H_
#define ELIB_RING_BUF_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// 环形缓冲区 (字节流 FIFO)
//--------------------------------------------------------------------+

typedef struct {
    uint8_t *buf;     // 数据存储区 (外部分配)
    size_t   size;    // 存储区总大小
    size_t   head;    // 写位置
    size_t   tail;    // 读位置
    size_t   count;   // 已缓存字节数
} ring_buf_t;

// 初始化 (buf[size] 由调用者分配, 生命周期由调用者管理)
void ring_buf_init(ring_buf_t *rb, uint8_t *buf, size_t size);

// 写入一个字节, 满时返回 false (不覆盖)
bool ring_buf_put(ring_buf_t *rb, uint8_t byte);

// 读取一个字节, 空时返回 false
bool ring_buf_get(ring_buf_t *rb, uint8_t *byte);

// 批量写入, 返回实际写入字节数
size_t ring_buf_write(ring_buf_t *rb, const uint8_t *data, size_t len);

// 批量读取, 返回实际读取字节数
size_t ring_buf_read(ring_buf_t *rb, uint8_t *data, size_t len);

// 查询
size_t ring_buf_count(const ring_buf_t *rb);
size_t ring_buf_free(const ring_buf_t *rb);
bool   ring_buf_empty(const ring_buf_t *rb);
bool   ring_buf_full(const ring_buf_t *rb);

// 重置为空
void ring_buf_reset(ring_buf_t *rb);

#ifdef __cplusplus
}
#endif

#endif
