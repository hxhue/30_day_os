#ifndef TREE_H
#define TREE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <stddef.h>

struct tree_node_t;

typedef struct tree_t {
  size_t element_size;
  size_t count;
  struct tree_node_t *root;
  // Sentinel. red == 0 and is read-only. Other properties are write-only.
  struct tree_node_t *nil;
  void *(*alloc)(size_t);
  void (*free)(void *);
  // Compares two payload, and returns a negative value when l < r, 0 when
  // l == r, a positive value when l > r.
  int (*cmp)(void *l, void *r);
} tree_t;

void tree_init(tree_t *tree, size_t element_size, void *(*alloc)(size_t),
               void (*free)(void *), int (*cmp)(void *, void *));
void tree_destroy(tree_t *tree);
void tree_remove(tree_t *tree, void *key);
void tree_insert(tree_t *tree, void *key);
size_t tree_size(const tree_t *tree);
int tree_empty(const tree_t *tree);
void *tree_find(const tree_t *tree, void *key);
void *tree_smallest_key(const tree_t *tree);
void *tree_largest_key(const tree_t *tree);
void *tree_next_key(const tree_t *tree, void *key);
void *tree_last_key(const tree_t *tree, void *key);
// Get size of a tree node for a specified element size.
size_t tree_get_node_size(size_t element_size);

#if (defined(__cplusplus))
}
#endif


#endif
