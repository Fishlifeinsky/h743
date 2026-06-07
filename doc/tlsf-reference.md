# TLSF v3.1 内存分配器参考文档

## 概述

TLSF (Two Level Segregated Fit) 是 O(1) 时间复杂度的实时内存分配器，由 Matthew Conte 实现，基于 Miguel Masmano 的论文。

核心思想：将空闲块按大小分级放入两级链表，通过硬件位操作指令（`__builtin_clz`）实现常数时间查找最适合的空闲块。

## 关键概念

### 类型

```c
typedef void* tlsf_t;   // TLSF 实例句柄，包含 1~N 个 pool
typedef void* pool_t;    // 内存池句柄，一块被 TLSF 管理的连续内存
```

### 控制块 (control_t)

每个 TLSF 实例需要一小块内存存放内部控制数据（位图 + 空闲链表头），大小约 3~4KB（可通过 `tlsf_size()` 查询）。

### 内存块 (block)

TLSF 将每个 pool 划分为连续的内存块，每个块有一个 8 字节（32-bit 平台）的头部：

```
[prev_phys_block (4B)] [size | free_flag (4B)] [user data...]
```

- `size` 包含了头部的总块大小，最低位为 `free_flag`
- 空闲块额外包含两个指针：`prev_free` 和 `next_free`，构成双向空闲链表

### 两级索引 (Two Level)

空闲块按大小分为两级：

- **第一级 (First Level, FL)**：对数粒度，将 2^4 ~ 2^31 的范围分为若干粗粒度区间
- **第二级 (Second Level, SL)**：将每个 FL 区间再线性细分为 SL_INDEX_COUNT 个子区间

查找时：`fl = __builtin_clz(size)` 直接定位到对应的 FL+SL 索引，取出该链表中的空闲块。

---

## API 参考

### 1. 创建/销毁实例

```c
tlsf_t tlsf_create(void* mem);
```

创建一个空的 TLSF 实例。`mem` 指向至少 `tlsf_size()` 字节的内存存放控制数据。返回的 `tlsf_t` 不包含任何 pool，需要后续 `tlsf_add_pool()` 添加。

```c
tlsf_t tlsf_create_with_pool(void* mem, size_t bytes);
```

一步创建 TLSF 实例并添加 pool。`mem` 前部用于控制数据，剩余空间作为第一个内存池。

```c
void tlsf_destroy(tlsf_t tlsf);
```

销毁 TLSF 实例。内部为空操作（控制数据结构在用户提供的内存中）。

---

### 2. 管理内存池

```c
pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, size_t bytes);
```

向已有 TLSF 实例添加一块内存池。`mem` 必须对齐到 `tlsf_align_size()` 边界，`bytes` 至少为 `tlsf_pool_overhead() + tlsf_block_size_min()`。

```c
void tlsf_remove_pool(tlsf_t tlsf, pool_t pool);
```

移除内存池（需先确保池内所有分配已释放）。

```c
pool_t tlsf_get_pool(tlsf_t tlsf);
```

获取 TLSF 实例的第一个 pool（通常也是唯一的 pool）。

---

### 3. 分配/释放

```c
void* tlsf_malloc(tlsf_t tlsf, size_t bytes);
```

分配 `bytes` 字节内存。内部会将请求大小对齐到 `tlsf_align_size()`。返回 NULL 表示无足够连续内存。

```c
void* tlsf_memalign(tlsf_t tlsf, size_t align, size_t bytes);
```

带对齐要求的分配。`align` 必须是 2 的幂且 ≥ `tlsf_align_size()`。

```c
void* tlsf_realloc(tlsf_t tlsf, void* ptr, size_t size);
```

重新分配内存。行为符合 C 标准 `realloc()`：
- `ptr == NULL` ⇒ 等同于 `tlsf_malloc()`
- `size == 0 && ptr != NULL` ⇒ 等同于 `tlsf_free()`
- 否则尝试原地扩展，失败则分配新块并拷贝数据

```c
void tlsf_free(tlsf_t tlsf, void* ptr);
```

释放 `ptr` 指向的内存块。释放后会自动与相邻空闲块合并（coalesce）。

```c
size_t tlsf_block_size(void* ptr);
```

返回 `ptr` 的内部块大小（含头部开销），通常大于 `malloc` 时请求的大小。**仅在 `ptr` 有效时结果有意义**。

---

### 4. 常量查询

```c
size_t tlsf_size(void);            // control_t 大小，约 3~4KB
size_t tlsf_align_size(void);       // 对齐粒度，32-bit 平台为 8 字节
size_t tlsf_block_size_min(void);  // 最小可用块大小
size_t tlsf_block_size_max(void);  // 最大可分配大小
size_t tlsf_pool_overhead(void);   // 每个 pool 的固定开销
size_t tlsf_alloc_overhead(void);  // 每次分配的块头开销 (8B)
```

使用示例 — 验证内存池是否足够：

```c
if (size < tlsf_pool_overhead() + tlsf_block_size_min()) {
    // 池太小，无法使用
}
```

---

### 5. 调试

```c
void tlsf_walk_pool(pool_t pool, tlsf_walker walker, void* user);
```

**遍历内存池中的所有块**，对每个块调用 `walker` 回调。

回调签名：
```c
typedef void (*tlsf_walker)(void* ptr, size_t size, int used, void* user);
```
- `ptr`：用户数据指针（同 `malloc` 返回的指针）
- `size`：块大小（含头部）
- `used`：1 表示已分配，0 表示空闲
- `user`：用户自定义上下文指针

**用途**：统计空闲/已用内存、打印内存布局、检测碎片。

**示例** — 统计空闲字节：
```c
static void count_free(void *ptr, size_t size, int used, void *user) {
    if (!used) *(size_t *)user += size;
}
size_t free_sz = 0;
tlsf_walk_pool(pool, count_free, &free_sz);
```

---

```c
int tlsf_check(tlsf_t tlsf);
```

**检测 TLSF 实例内部数据结构完整性**。遍历所有 FL/SL 位图和空闲链表，验证：
- 第一级位图与第二级位图一致
- 每个非空链表确实包含至少一个空闲块
- 空闲块的大小确实落在此链表的区间内

返回 0 表示无错误，非 0 表示发现了不一致（数据损坏）。

**用途**：内存越界破坏 TLSF 元数据后的故障定位。

---

```c
int tlsf_check_pool(pool_t pool);
```

**检测单个内存池的物理完整性**。遍历池中所有块，验证：
- 相邻块的前后指针一致（`block->prev_phys_block.next == block`）
- 空闲块不在已分配链表上
- 没有两个相邻的空闲块（应已合并）

返回 0 表示无错误，非 0 表示块链表损坏。

**用途**：配合 `tlsf_walk_pool` 定位损坏的具体块。

---

## 性能特性

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| `tlsf_malloc` | O(1) | 常数时间查找空闲块 |
| `tlsf_free` | O(1) | 常数时间合并相邻空闲块 |
| `tlsf_realloc` | O(n) | 需要拷贝时线性于块大小 |
| `tlsf_check` | O(b) | b = 总块数，线性 |
| `tlsf_walk_pool` | O(b) | b = 池中总块数，线性 |

## 内存布局示意

```
TLSF 实例 (由 tlsf_create_with_pool 一步构建):

  mem →
  ┌──────────────────────────────────────┐
  │  control_t (~3-4KB)                 │ ← 位图 + 空闲链表头
  │  - fl_bitmap, sl_bitmap[][]         │
  │  - blocks[][] (空闲链表头)           │
  ├──────────────────────────────────────┤
  │  pool (剩余空间)                      │
  │  ┌────────────────────────────────┐  │
  │  │ [header]  free block (sentinel)│  │ ← 占 2*header_overhead
  │  ├────────────────────────────────┤  │
  │  │ [header]  free                │  │ ← 可分配的空闲空间
  │  │           ...                 │  │
  │  ├────────────────────────────────┤  │
  │  │ [header]  free block (sentinel)│  │ ← 占 1*header_overhead
  │  └────────────────────────────────┘  │
  └──────────────────────────────────────┘
```

## 注意事项

1. **对齐要求**：`tlsf_add_pool` 的 `mem` 必须对齐到 `tlsf_align_size()`（8 字节）
2. **最小池大小**：`tlsf_pool_overhead() + tlsf_block_size_min()`
3. **线程安全**：TLSF 本身无锁，多线程环境需在调用层加锁
4. **块大小查询**：`tlsf_block_size()` 返回内部块的实际大小，对已释放的指针调用结果是未定义的
5. **realloc 行为**：`tlsf_realloc(NULL, size)` = `malloc`，`tlsf_realloc(ptr, 0)` = `free`

## 参考链接

- 项目主页：http://tlsf.baisoku.org
- 原始论文：http://www.gii.upv.es/tlsf/main/docs
