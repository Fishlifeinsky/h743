#include "elib/ring_buffer.h"

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

void ring_buf_init(ring_buf_t *rb, uint8_t *buf, size_t size) {
    rb->buf   = buf;
    rb->size  = size;
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
}

bool ring_buf_put(ring_buf_t *rb, uint8_t byte) {
    if (rb->count >= rb->size) return false;
    rb->buf[rb->head] = byte;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    return true;
}

bool ring_buf_get(ring_buf_t *rb, uint8_t *byte) {
    if (rb->count == 0) return false;
    *byte = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    return true;
}

size_t ring_buf_write(ring_buf_t *rb, const uint8_t *data, size_t len) {
    size_t n = 0;
    while (n < len && ring_buf_put(rb, data[n])) {
        n++;
    }
    return n;
}

size_t ring_buf_read(ring_buf_t *rb, uint8_t *data, size_t len) {
    size_t n = 0;
    while (n < len && ring_buf_get(rb, &data[n])) {
        n++;
    }
    return n;
}

size_t ring_buf_count(const ring_buf_t *rb) {
    return rb->count;
}

size_t ring_buf_free(const ring_buf_t *rb) {
    return rb->size - rb->count;
}

bool ring_buf_empty(const ring_buf_t *rb) {
    return rb->count == 0;
}

bool ring_buf_full(const ring_buf_t *rb) {
    return rb->count >= rb->size;
}

void ring_buf_reset(ring_buf_t *rb) {
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
}
