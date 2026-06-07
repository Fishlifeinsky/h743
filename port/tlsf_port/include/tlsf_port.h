#ifndef BSP_TLSF_PORT_H_
#define BSP_TLSF_PORT_H_

#include "tlsf.h"
#include "port/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// TLSF 内存池类型
//--------------------------------------------------------------------+
typedef enum {
    TLSF_PORT_SDRAM   = 0,  // 外部 SDRAM (0xC0000000, 16MB)
    TLSF_PORT_SRAM_D1 = 1,  // 内部 SRAM D1 (0x24000000, 512KB 剩余空间)
    TLSF_PORT_POOL_COUNT
} tlsf_port_pool_t;

//--------------------------------------------------------------------+
// 外部接口 — 基础 alloc/free
//--------------------------------------------------------------------+

void  tlsf_port_init(void);
void  tlsf_port_rtos_init(void);
void *tlsf_port_malloc(size_t size, tlsf_port_pool_t pool);
void *tlsf_port_realloc(void *ptr, size_t size);
void  tlsf_port_free(void *ptr);
tlsf_t tlsf_port_get(tlsf_port_pool_t pool);

//--------------------------------------------------------------------+
// 外部接口 — 内存池信息
//--------------------------------------------------------------------+

size_t tlsf_port_get_total(tlsf_port_pool_t pool);
size_t tlsf_port_get_free(tlsf_port_pool_t pool);
size_t tlsf_port_get_used(tlsf_port_pool_t pool);
const char * tlsf_port_get_name(tlsf_port_pool_t pool);
void tlsf_port_get_stats(tlsf_port_pool_t pool,
                         size_t *largest_free,
                         size_t *free_cnt,
                         size_t *used_cnt);

//--------------------------------------------------------------------+
// 单元测试
//--------------------------------------------------------------------+

#if USE_BSP_UNIT_TESTING
void tlsf_port_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
