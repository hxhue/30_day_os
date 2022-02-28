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

// 1. Creation and destruction

void tree_init(tree_t *tree, size_t element_size, void *(*alloc)(size_t),
               void (*free)(void *), int (*cmp)(void *, void *));
void tree_destroy(tree_t *tree);

// 2. Modifications

// Returns 0 on success, -1 on failure (when key is not in the tree).
int tree_remove(tree_t *tree, void *key);

// Returns the key in the tree after insertion. Insertion will not happen
// if the key can be found in the tree, the equivalent key is returned
// instead. If insertion fails due to memory shortage, 0 will be returned.
void *tree_insert(tree_t *tree, void *key);

// Updates tree structure after key is modified. key must belongs to an existing
// node in the tree. If this function is called, addresses of internal keys are
// likely to change. Otherwise, the addresses of keys are stable.
void tree_update(tree_t *tree, void *key);

// 3. Property queries

// Get size of a tree node for a specified element size.
size_t tree_get_node_size(size_t element_size);
size_t tree_size(const tree_t *tree);
int tree_empty(const tree_t *tree);

// 4. Search and iteration

void *tree_find(const tree_t *tree, void *key);
void *tree_smallest_key(const tree_t *tree);
void *tree_largest_key(const tree_t *tree);
void *tree_next_key(const tree_t *tree, void *key);
void *tree_last_key(const tree_t *tree, void *key);

#if (defined(__cplusplus))
}
#endif


#endif
