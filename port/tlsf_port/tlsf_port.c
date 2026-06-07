#include "tlsf_port.h"
#include "mem.h"

#if BSP_FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "semphr.h"
#endif

//--------------------------------------------------------------------+
// 内部全局变量
//--------------------------------------------------------------------+

static tlsf_t  g_tlsf[TLSF_PORT_POOL_COUNT];
static size_t  g_pool_total[TLSF_PORT_POOL_COUNT];   /* 池总大小 */
static size_t  g_pool_used[TLSF_PORT_POOL_COUNT];    /* 已使用量 */

static const char * g_pool_name[TLSF_PORT_POOL_COUNT] = {
    [TLSF_PORT_SDRAM]   = "SDRAM",
    [TLSF_PORT_SRAM_D1] = "SRAM_D1",
};

#if BSP_FREERTOS_ENABLED
static SemaphoreHandle_t g_mutex;
static StaticSemaphore_t g_mutex_buffer;

#define TLSF_LOCK()   do { if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY); } while(0)
#define TLSF_UNLOCK() do { if (g_mutex) xSemaphoreGive(g_mutex); } while(0)
#else
#define TLSF_LOCK()   ((void)0)
#define TLSF_UNLOCK() ((void)0)
#endif

// 各池地址范围 (用于 free/realloc 自动识别)
static const uint32_t g_pool_start[TLSF_PORT_POOL_COUNT] = {
    [TLSF_PORT_SDRAM]   = MEM_SDRAM_START,
    [TLSF_PORT_SRAM_D1] = MEM_SRAM_D1_START,
};

static const uint32_t g_pool_end[TLSF_PORT_POOL_COUNT] = {
    [TLSF_PORT_SDRAM]   = MEM_SDRAM_START + (16 * 1024 * 1024),
    [TLSF_PORT_SRAM_D1] = MEM_SRAM_D1_START + MEM_SRAM_D1_TOTAL_SIZE,
};

//--------------------------------------------------------------------+
// 静态函数
//--------------------------------------------------------------------+

static tlsf_port_pool_t get_pool_by_addr(void *ptr) {
    uint32_t addr = (uint32_t)ptr;
    for (int i = 0; i < TLSF_PORT_POOL_COUNT; i++) {
        if (g_tlsf[i] && addr >= g_pool_start[i] &&
            addr < g_pool_end[i]) {
            return (tlsf_port_pool_t)i;
        }
    }
    return TLSF_PORT_POOL_COUNT;
}

//--------------------------------------------------------------------+
// 外部接口 — 初始化
//--------------------------------------------------------------------+

void tlsf_port_init(void) {
    // 1. D1 池
    {
        uint32_t d1_free = MEM_SRAM_D1_FREE_SIZE;
        uint32_t align = tlsf_align_size();
        uint32_t misalign = MEM_SRAM_D1_FREE_START % align;
        uint32_t start = MEM_SRAM_D1_FREE_START + (misalign ? align - misalign : 0);
        uint32_t size = d1_free - (start - MEM_SRAM_D1_FREE_START);
        size &= ~(align - 1);

        if (size < tlsf_pool_overhead() + tlsf_block_size_min()) {
            DEBUG_PRINT("TLSF: SRAM D1 too small, skip\r\n");
            g_tlsf[TLSF_PORT_SRAM_D1] = NULL;
            g_pool_total[TLSF_PORT_SRAM_D1] = 0;
        } else {
            g_tlsf[TLSF_PORT_SRAM_D1] = tlsf_create_with_pool((void *)start, size);
            g_pool_total[TLSF_PORT_SRAM_D1] = size;
            DEBUG_PRINT("TLSF: SRAM D1 pool %p +%lu\r\n", (void *)start, size);
        }
    }

    // 2. SDRAM 池
    g_tlsf[TLSF_PORT_SDRAM] = tlsf_create_with_pool(
        (void *)MEM_SDRAM_FREE_START, MEM_SDRAM_FREE_SIZE);
    g_pool_total[TLSF_PORT_SDRAM] = MEM_SDRAM_FREE_SIZE;
    DEBUG_PRINT("TLSF: SDRAM pool %p +%lu\r\n",
                (void *)MEM_SDRAM_FREE_START, MEM_SDRAM_FREE_SIZE);
}

void tlsf_port_rtos_init(void) {
#if BSP_FREERTOS_ENABLED
    g_mutex = xSemaphoreCreateMutexStatic(&g_mutex_buffer);
#endif
}

//--------------------------------------------------------------------+
// 外部接口 — alloc / free
//--------------------------------------------------------------------+

void *tlsf_port_malloc(size_t size, tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT || !g_tlsf[pool]) return NULL;
    TLSF_LOCK();
    void *p = tlsf_malloc(g_tlsf[pool], size);
    if (p) g_pool_used[pool] += tlsf_block_size(p);
    TLSF_UNLOCK();
    return p;
}

void tlsf_port_free(void *ptr) {
    if (!ptr) return;
    tlsf_port_pool_t pool = get_pool_by_addr(ptr);
    if (pool >= TLSF_PORT_POOL_COUNT) {
        DEBUG_PRINT("TLSF: free bad ptr %p\r\n", ptr);
        return;
    }
    TLSF_LOCK();
    g_pool_used[pool] -= tlsf_block_size(ptr);
    tlsf_free(g_tlsf[pool], ptr);
    TLSF_UNLOCK();
}

void *tlsf_port_realloc(void *ptr, size_t size) {
    if (!ptr) return tlsf_port_malloc(size, BSP_MALLOC_DEFAULT_POOL);
    tlsf_port_pool_t pool = get_pool_by_addr(ptr);
    if (pool >= TLSF_PORT_POOL_COUNT) {
        DEBUG_PRINT("TLSF: realloc bad ptr %p\r\n", ptr);
        return NULL;
    }
    TLSF_LOCK();
    size_t old_size = tlsf_block_size(ptr);
    void *p = tlsf_realloc(g_tlsf[pool], ptr, size);
    if (p) g_pool_used[pool] += tlsf_block_size(p) - old_size;
    TLSF_UNLOCK();
    return p;
}

tlsf_t tlsf_port_get(tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT) return NULL;
    return g_tlsf[pool];
}

//--------------------------------------------------------------------+
// 外部接口 — 内存池信息
//--------------------------------------------------------------------+

size_t tlsf_port_get_total(tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT || !g_tlsf[pool]) return 0;
    return g_pool_total[pool];
}

size_t tlsf_port_get_free(tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT || !g_tlsf[pool]) return 0;
    return g_pool_total[pool] - g_pool_used[pool];
}

size_t tlsf_port_get_used(tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT) return 0;
    return g_pool_used[pool];
}

const char * tlsf_port_get_name(tlsf_port_pool_t pool) {
    if (pool >= TLSF_PORT_POOL_COUNT) return "?";
    return g_pool_name[pool];
}
