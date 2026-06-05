#ifndef ELIB_LIST_H_
#define ELIB_LIST_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 单向链表节点
typedef struct elist_node {
    void              *data;
    struct elist_node *next;
} elist_node_t;

// 单向链表
typedef struct elist {
    elist_node_t *head;
    elist_node_t *tail;
    size_t        size;
} elist_t;

// container_of: 从嵌入的节点指针获取所在结构体
#define elist_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// 初始化 / 清空
void   elist_init(elist_t *list);
void   elist_clear(elist_t *list);

// 属性
size_t elist_size(const elist_t *list);
bool   elist_empty(const elist_t *list);

// ---- 节点操作 ----
elist_node_t *elist_push_front(elist_t *list, void *data);
elist_node_t *elist_push_back(elist_t *list, void *data);
elist_node_t *elist_insert_after(elist_t *list, elist_node_t *node, void *data);
void          elist_remove(elist_t *list, elist_node_t *node);
elist_node_t *elist_find(const elist_t *list, const void *data,
                         int (*cmp)(const void *, const void *));
elist_node_t *elist_find_node(const elist_t *list, const void *data);
void          elist_remove_data(elist_t *list, const void *data);
void          elist_foreach(const elist_t *list, void (*fn)(void *));

void *elist_pop_front(elist_t *list);
void *elist_pop_back(elist_t *list);
void *elist_front(const elist_t *list);
void *elist_back(const elist_t *list);

// ---- 索引操作 ----
elist_node_t *elist_at(const elist_t *list, size_t index);
void         *elist_get(const elist_t *list, size_t index);
void          elist_set(elist_t *list, size_t index, void *data);
elist_node_t *elist_insert_at(elist_t *list, size_t index, void *data);
void          elist_remove_at(elist_t *list, size_t index);

#ifdef __cplusplus
}
#endif

#endif
