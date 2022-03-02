#include "list.h"
#include "support/list.h"
#include <stddef.h>
#include <string.h>

// Memory layout: node head (2 pointers) + data
struct list_node_t {
  list_node_t *prev, *next;
};

static inline size_t node_size(size_t element_size) {
  return sizeof(list_node_t) + element_size;
}

static inline void *get_data(const list_node_t *node) {
  return (char *)node + sizeof(list_node_t);
}

static inline list_node_t *get_node(const void *data) {
  return (list_node_t *)((char *)data - sizeof(list_node_t));
}

////////////////////// Non-static (public) functions //////////////////////

size_t list_get_node_size(size_t element_size) {
  return node_size(element_size);
}

void *list_get_value(const list_node_t *node) { return get_data(node); }

void list_init(list_t *list, size_t element_size, void *(*alloc)(size_t),
               void (*free)(void *)) {
  list->element_size = element_size;
  list->count = 0;
  list->free = free;
  list->alloc = alloc;
  list->end = (list_node_t *)alloc(node_size(element_size));
  list->end->prev = list->end->next = list->end;
}

list_node_t *list_make_node(list_t *list, void *value) {
  list_node_t *node = (list_node_t *)list->alloc(node_size(list->element_size));
  if (!node) {
    return (list_node_t *)0;
  }
  node->prev = node->next = (list_node_t *)0;
  memcpy(get_data(node), value, list->element_size);
  return node;
}

list_node_t *list_push_back(list_t *list, list_node_t *node) {
  list_node_t *back = list->end->prev;
  node->prev = back;
  node->next = list->end;
  back->next = node;
  list->end->prev = node;
  list->count++;
  return node;
}

list_node_t *list_push_front(list_t *list, list_node_t *node) {
  list_node_t *front = list->end->next;
  node->prev = list->end;
  node->next = front;
  list->end->next = node;
  front->prev = node;
  list->count++;
  return node;
}

list_node_t *list_pop_back(list_t *list) {
  list_node_t *node = list->end->prev; 
  return (node == list->end) ? NULL : list_unlink(list, node);
}

list_node_t *list_pop_front(list_t *list) {
  list_node_t *node = list->end->next;
  return (node == list->end) ? NULL : list_unlink(list, node);
}

size_t list_size(list_t *list) { return list->count; }

list_node_t *list_front_node(const list_t *list) { 
  list_node_t *node = list->end->next;
  return (node == list->end) ? NULL : node;
}

list_node_t *list_back_node(const list_t *list) { 
  list_node_t *node = list->end->prev; 
  return (node == list->end) ? NULL : node;
}

list_node_t *list_begin(const list_t *list) {
  return list->end->next;
}

list_node_t *list_rbegin(const list_t *list) {
  return list->end->prev;
}

list_node_t *list_end(const list_t *list) { 
  return list->end; 
}

list_node_t *list_rend(const list_t *list) { 
  return list->end; 
}

// Unlink the node, but do not erase the data stored in it.
list_node_t *list_unlink(list_t *list, list_node_t *node) {
  if (node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node->prev = (list_node_t *)0;
    list->count--;
  }
  return node;
}

// Erase the iterator and data associating with it. The node pointed by the
// iterator must be unlinked first.
void list_free(const list_t *list, list_node_t *node) {
  list->free(node);
}

// Returns the inserted node.
list_node_t *list_insert_before(list_t *list, list_node_t *node,
                                     list_node_t *pos) {
  node->next = pos;
  node->prev = pos->prev;
  pos->prev->next = node;
  pos->prev = node;
  list->count++;
  return node;
}

// Returns the inserted node.
list_node_t *list_insert_after(list_t *list, list_node_t *node,
                                    list_node_t *pos) {
  node->next = pos->next;
  node->prev = pos;
  pos->next->prev = node;
  pos->next = node;
  list->count++;
  return node;
}

list_node_t *list_prev_node(const list_t *list, const list_node_t *node) {
  list_node_t *p = node->prev;
  return (p == list->end) ? NULL : p;
}

list_node_t *list_next_node(const list_t *list, const list_node_t *node) {
  list_node_t *p = node->next;
  return (p == list->end) ? NULL : p;
}

void list_destory(list_t *list) {
  list_node_t *end = list->end;
  list_node_t *p = end->next;
  list_node_t *next;
  for (; p != end; p = next) {
    next = p->next;
    list->free(p);
  }
  list->free(end);
  list->count = 0;
}
