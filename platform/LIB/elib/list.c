#include "elib/list.h"
#include <stdlib.h>

static elist_node_t *node_new(void *data) {
    elist_node_t *node = (elist_node_t *)malloc(sizeof(elist_node_t));
    if (node) {
        node->data = data;
        node->next = NULL;
    }
    return node;
}

static void node_free(elist_node_t *node) {
    free(node);
}

void elist_init(elist_t *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void elist_clear(elist_t *list) {
    elist_node_t *cur = list->head;
    while (cur) {
        elist_node_t *next = cur->next;
        node_free(cur);
        cur = next;
    }
    elist_init(list);
}

size_t elist_size(const elist_t *list) {
    return list->size;
}

bool elist_empty(const elist_t *list) {
    return list->size == 0;
}

// ---- 节点操作 ----

elist_node_t *elist_push_front(elist_t *list, void *data) {
    elist_node_t *node = node_new(data);
    if (!node) return NULL;

    node->next = list->head;
    list->head = node;
    if (!list->tail) {
        list->tail = node;
    }
    list->size++;
    return node;
}

elist_node_t *elist_push_back(elist_t *list, void *data) {
    elist_node_t *node = node_new(data);
    if (!node) return NULL;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
    return node;
}

elist_node_t *elist_insert_after(elist_t *list, elist_node_t *node, void *data) {
    if (!node) return NULL;

    elist_node_t *new_node = node_new(data);
    if (!new_node) return NULL;

    new_node->next = node->next;
    node->next = new_node;
    if (list->tail == node) {
        list->tail = new_node;
    }
    list->size++;
    return new_node;
}

void elist_remove(elist_t *list, elist_node_t *node) {
    if (!node) return;

    if (list->head == node) {
        list->head = node->next;
    } else {
        elist_node_t *prev = list->head;
        while (prev && prev->next != node) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = node->next;
        }
    }

    if (list->tail == node) {
        if (list->head == NULL || list->head == node) {
            list->tail = NULL;
        } else {
            elist_node_t *cur = list->head;
            while (cur->next != node) {
                cur = cur->next;
            }
            list->tail = cur;
        }
    }

    node_free(node);
    list->size--;
}

elist_node_t *elist_find(const elist_t *list, const void *data,
                          int (*cmp)(const void *, const void *)) {
    elist_node_t *cur = list->head;
    while (cur) {
        if (cmp(cur->data, data) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

elist_node_t *elist_find_node(const elist_t *list, const void *data) {
    elist_node_t *cur = list->head;
    while (cur) {
        if (cur->data == data) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void elist_remove_data(elist_t *list, const void *data) {
    elist_node_t *node = elist_find_node(list, data);
    elist_remove(list, node);
}

void elist_foreach(const elist_t *list, void (*fn)(void *)) {
    elist_node_t *cur = list->head;
    while (cur) {
        fn(cur->data);
        cur = cur->next;
    }
}

void *elist_pop_front(elist_t *list) {
    if (!list->head) return NULL;

    elist_node_t *node = list->head;
    void *data = node->data;

    list->head = node->next;
    if (!list->head) {
        list->tail = NULL;
    }
    node_free(node);
    list->size--;
    return data;
}

void *elist_pop_back(elist_t *list) {
    if (!list->tail) return NULL;

    if (list->head == list->tail) {
        void *data = list->tail->data;
        node_free(list->tail);
        list->head = NULL;
        list->tail = NULL;
        list->size--;
        return data;
    }

    elist_node_t *prev = list->head;
    while (prev->next != list->tail) {
        prev = prev->next;
    }

    void *data = list->tail->data;
    node_free(list->tail);
    prev->next = NULL;
    list->tail = prev;
    list->size--;
    return data;
}

void *elist_front(const elist_t *list) {
    return list->head ? list->head->data : NULL;
}

void *elist_back(const elist_t *list) {
    return list->tail ? list->tail->data : NULL;
}

// ---- 索引操作 ----

elist_node_t *elist_at(const elist_t *list, size_t index) {
    if (index >= list->size) return NULL;

    elist_node_t *cur = list->head;
    for (size_t i = 0; i < index; i++) {
        cur = cur->next;
    }
    return cur;
}

void *elist_get(const elist_t *list, size_t index) {
    elist_node_t *node = elist_at(list, index);
    return node ? node->data : NULL;
}

void elist_set(elist_t *list, size_t index, void *data) {
    elist_node_t *node = elist_at(list, index);
    if (node) {
        node->data = data;
    }
}

elist_node_t *elist_insert_at(elist_t *list, size_t index, void *data) {
    if (index > list->size) return NULL;

    if (index == 0) {
        return elist_push_front(list, data);
    }
    if (index == list->size) {
        return elist_push_back(list, data);
    }

    elist_node_t *prev = elist_at(list, index - 1);
    return elist_insert_after(list, prev, data);
}

void elist_remove_at(elist_t *list, size_t index) {
    elist_node_t *node = elist_at(list, index);
    elist_remove(list, node);
}
