#ifndef LIST_H
#define LIST_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <stddef.h>

typedef struct list_node_t list_node_t;

// Unless the node is the head of some list, a node can be used across different
// lists, moving from one to another, given that these lists **SHARE THE SAME**
// alloc() and free() function pointers.
typedef struct list_t {
  size_t element_size;
  size_t count;
  struct list_node_t *end;
  void *(*alloc)(size_t);
  void (*free)(void *);
} list_t;

size_t list_get_node_size(size_t element_size);
size_t list_size(list_t *list);

void list_init(list_t *list, size_t element_size, void *(*alloc)(size_t),
               void (*free)(void *));
void list_destory(list_t *list);

list_node_t *list_make_node(list_t *list, void *value);
void        *list_get_value(const list_node_t *node);
list_node_t *list_unlink(list_t *list, list_node_t *node);
void         list_free(const list_t *list, list_node_t *node);

list_node_t *list_push_back(list_t *list, list_node_t *node);
list_node_t *list_push_front(list_t *list, list_node_t *node);
list_node_t *list_pop_back(list_t *list);
list_node_t *list_pop_front(list_t *list);
list_node_t *list_insert_before(list_t *list, list_node_t *node,
                                list_node_t *pos);
list_node_t *list_insert_after(list_t *list, list_node_t *node,
                               list_node_t *pos);

// C++ STL style begin()/end(). The end of a list shouldn't be modified. There
// is no guarantee that end of a list is (and most likely will not be) NULL.
// To move to the next node, use assignment "node = node->next" when you
// iterate through range [list_begin(list), list_end(list)), or "node =
// node->prev" when you iterate through range [list_rbegin(list),
// list_rend(list)).
list_node_t *list_begin(const list_t *list);
list_node_t *list_end(const list_t *list);
list_node_t *list_rbegin(const list_t *list);
list_node_t *list_rend(const list_t *list);

// NULL terminating style iteration methods.
list_node_t *list_front_node(const list_t *list);
list_node_t *list_back_node(const list_t *list);
list_node_t *list_prev_node(const list_t *list, const list_node_t *node);
list_node_t *list_next_node(const list_t *list, const list_node_t *node);

#if (defined(__cplusplus))
}
#endif


#endif
