#include "tree.h"
#include "debug.h"
#include <string.h>

#ifdef __GNUC__
  #define STRUCT_PACK_START 
  #define STRUCT_PACK_END   __attribute__((__packed__))
#elif defined (_MSC_VER)
  #define STRUCT_PACK_START __pragma(pack(push, 1)) 
  #define STRUCT_PACK_END   __pragma(pack(pop))
#else
  #define STRUCT_PACK_START
  #define STRUCT_PACK_END  
#endif

STRUCT_PACK_START
struct tree_node_t {
  int red;        // 1 for red, 0 for black
  struct tree_node_t *left, *right, *parent;
}
STRUCT_PACK_END;

typedef struct tree_node_t tree_node_t;

// Element memory layout:
// sizeof(tree_node_t) bytes for header
//    Node header data
//    Paddings (when "packed" attribute is not availble)
// "element_size" bytes for an element

static inline void *tree_node_get_key(tree_node_t *node) {
  return (char *) node + sizeof(tree_node_t);
}

static inline tree_node_t *tree_node_from_key(void *key) {
  return (tree_node_t *)((char *)key - sizeof(tree_node_t));
}

static inline size_t tree_get_node_size_inline(size_t element_size) {
  return ((sizeof(tree_node_t) + element_size) + 7) / 8 * 8;
}

size_t tree_get_node_size(size_t element_size) {
  return tree_get_node_size_inline(element_size);
}

// Create a single tree node. Returns nil if memory is not enough.
static tree_node_t *tree_node_new(tree_t *tree, void *key, tree_node_t *parent) {
  tree_node_t *node =
      (tree_node_t *)tree->alloc(tree_get_node_size_inline(tree->element_size));
  
  if (!node) {
    return tree->nil;
  }
  
  node->left = node->right = tree->nil;
  node->red = 1;
  node->parent = parent;
  memcpy(tree_node_get_key(node), key, tree->element_size);
  return node;
}

// Rotate the tree and fix link from upstream. Color needs to be fixed by other
// procedures. This function assumes x->right != tree->nil and tree->root->parent
// == tree->nil.
static void tree_node_rotate_left(tree_t *tree, tree_node_t *x) {
  tree_node_t *y = x->right, *nil = tree->nil;
  x->right = y->left;
  if (y->left != nil) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == nil)          tree->root = y;
  else if (x == x->parent->left) x->parent->left = y;
  else                           x->parent->right = y;
  y->left = x;
  x->parent = y;
}

// Rotate the tree and fix link from upstream. Color needs to be fixed by other
// procedures. This function assumes x->left != tree->nil and tree->root->parent
// == tree->nil.
static void tree_node_rotate_right(tree_t *tree, tree_node_t *x) {
  tree_node_t *y = x->left, *nil = tree->nil;
  x->left = y->right;
  if (y->right != nil) {
    y->right->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == nil)          tree->root = y;
  else if (x == x->parent->left) x->parent->left = y;
  else                           x->parent->right = y;
  y->right = x;
  x->parent = y;
}

static void tree_node_insert_balance(tree_t *tree, tree_node_t *x) {
  // 1. x is always red.
  // 2. There is no invalid node except the newly inserted one.
  while (x != tree->root && x->parent->red) {
    tree_node_t *pp = x->parent->parent;
    if (x->parent == pp->left) {
      tree_node_t *y = pp->right;
      if (y->red) {
        x->parent->red = 0;
        y->red = 0;
        pp->red = 1;
        x = pp;
      } else {
        if (x == x->parent->right) {
          x = x->parent;
          tree_node_rotate_left(tree, x);
        }
        x->parent->red = 0;
        pp->red = 1;
        tree_node_rotate_right(tree, pp);
      }
    } else {
      tree_node_t *y = pp->left;
      if (y->red) {
        x->parent->red = 0;
        y->red = 0;
        pp->red = 1;
        x = pp;
      } else {
        if (x == x->parent->left) {
          x = x->parent;
          tree_node_rotate_right(tree, x);
        }
        x->parent->red = 0;
        pp->red = 1;
        tree_node_rotate_left(tree, pp);
      }
    }
  }
}

// Returns the node in the tree after insertion. If memory is not enough, nil
// is returned instead. Requirements: tree != 0, key != 0.
static tree_node_t *tree_node_insert(tree_t *tree, void *key) {
  int (*cmp)(void *, void *) = tree->cmp;
  tree_node_t *nil = tree->nil;
  tree_node_t *parent = nil, *cur = tree->root;

  int result = 0;
  while (cur != nil) {
    result = cmp(key, tree_node_get_key(cur));
    if (result < 0)      parent = cur, cur = cur->left;
    else if (result > 0) parent = cur, cur = cur->right;
    else                 return cur;// No insertion
  }

  cur = tree_node_new(tree, key, parent);

  // Out of memory
  if (cur == nil) {
    return nil;
  }

  if (result < 0)        parent->left = cur;
  else if (result > 0)   parent->right = cur;
  else                   tree->root = cur;

  tree_node_insert_balance(tree, cur);

  tree->count++;
  return cur;
}

static tree_node_t *tree_node_find(const tree_t *tree, tree_node_t *node,
                            void *key) {
  int (*cmp)(void *, void *) = tree->cmp;
  tree_node_t *nil = tree->nil;

  while (node != nil) {
    int result = cmp(key, tree_node_get_key(node));
    if (result < 0)      node = node->left;
    else if (result > 0) node = node->right;
    else                 return node;
  }

  return node;
}

void *tree_insert(tree_t *tree, void *key) {
  tree_node_t *node = tree_node_insert(tree, key);
  tree->root->red = 0;
  tree->root->parent = tree->nil;
  return node == tree->nil ? (void *)0 : tree_node_get_key(node);
}

size_t tree_size(const tree_t *tree) {
   return tree->count;
}

int tree_empty(const tree_t *tree) {
  return !tree || tree->root == tree->nil;
}

void *tree_find(const tree_t *tree, void *key) {
  tree_node_t *node = tree_node_find(tree, tree->root, key);
  return node == tree->nil ? (void *)0 : tree_node_get_key(node);
}

// Requirement(s): tree != 0, cur != nil
static inline tree_node_t *tree_node_leftmost(const tree_t *tree,
                                              tree_node_t *cur) {
  tree_node_t *nil = tree->nil;
  while (cur->left != nil) {
    cur = cur->left;
  }
  return cur;
}

// Requirement(s): tree != 0, cur != nil
static inline tree_node_t *tree_node_rightmost(const tree_t *tree,
                                               tree_node_t *cur) {
  tree_node_t *nil = tree->nil;
  while (cur->right != nil) {
    cur = cur->right;
  }
  return cur;
}

// Returns the address of the first key.
// Requirement(s): The key should not be written!
void *tree_smallest_key(const tree_t *tree) {
  if (!tree || tree->root == tree->nil) {
    return (void *) 0;
  }
  tree_node_t *h = tree_node_leftmost(tree, tree->root);
  return tree_node_get_key(h);
}

// Returns the address of the last key, not the off-the-end key!
// Requirement(s): The key should not be written!
void *tree_largest_key(const tree_t *tree) {
  if (!tree || tree->root == tree->nil) {
    return (void *) 0;
  }
  tree_node_t *h = tree_node_rightmost(tree, tree->root);
  return tree_node_get_key(h);
}

// Requirement(s): "key" should be a node in "tree".
void *tree_next_key(const tree_t *tree, void *key) {
  tree_node_t *node = tree_node_from_key(key);
  tree_node_t *nil = tree->nil;
  if (node->right != nil)
    return tree_node_get_key(tree_node_leftmost(tree, node->right));
  while (node->parent != nil) {
    if (node == node->parent->left) {
      return tree_node_get_key(node->parent);
    }
    node = node->parent;
  }
  return (void *) 0;
}

// Requirement(s): "key" should be a node in "tree".
void *tree_prev_key(const tree_t *tree, void *key) {
  tree_node_t *node = tree_node_from_key(key);
  tree_node_t *nil = tree->nil;
  if (node->left != nil)
    return tree_node_get_key(tree_node_rightmost(tree, node->left));
  while (node->parent != nil) {
    if (node == node->parent->right) {
      return tree_node_get_key(node->parent);
    }
    node = node->parent;
  }

  return (void *) 0;
}

void tree_init(tree_t *tree, size_t element_size, void *(*alloc)(size_t),
               void (*free)(void *), int (*cmp)(void *, void *)) {
  tree->alloc = alloc;
  tree->free = free;
  tree->cmp = cmp;
  tree->element_size = element_size;
  tree->count = 0;
  tree->nil = (tree_node_t *)alloc(tree_get_node_size_inline(element_size));
  tree->nil->red = 0;
  tree->root = tree->nil;
}

static void tree_node_remove_balance(tree_t *tree, tree_node_t *x) {
  while (x != tree->root && !x->red) {
    if (x == x->parent->left) {
      tree_node_t *w = x->parent->right;
      if (w->red) {
        w->red = 0;
        x->parent->red = 1;
        tree_node_rotate_left(tree, x->parent);
        w = x->parent->right;
      }
      if (!w->left->red && !w->right->red) {
        w->red = 1;
        x = x->parent;
      } else {
        if (!w->right->red) {
          w->left->red = 0;
          w->red = 1;
          tree_node_rotate_right(tree, w);
          w = x->parent->right;
        }
        w->red = x->parent->red;
        x->parent->red = 0;
        w->right->red = 0;
        tree_node_rotate_left(tree, x->parent);
        x = tree->root;
      }
    } else {
      tree_node_t *w = x->parent->left;
      if (w->red) {
        w->red = 0;
        x->parent->red = 1;
        tree_node_rotate_right(tree, x->parent);
        w = x->parent->left;
      }
      if (!w->right->red && !w->left->red) {
        w->red = 1;
        x = x->parent;
      } else {
        if (!w->left->red) {
          w->right->red = 0;
          w->red = 1;
          tree_node_rotate_left(tree, w);
          w = x->parent->left;
        }
        w->red = x->parent->red;
        x->parent->red = 0;
        w->left->red = 0;
        tree_node_rotate_right(tree, x->parent);
        x = tree->root;
      }
    }
  }
  x->red = 0;
}

// Let y replace x. x is then detached from the tree. x must exist, but y can be
// nil.
static void tree_node_replace(tree_t *rbt, tree_node_t *x, tree_node_t *y) {
  if (x->parent == rbt->nil) {
    rbt->root = y;
  } else {
    if (x == x->parent->left) x->parent->left = y;
    else                      x->parent->right = y;
  }
  y->parent = x->parent;
}

// z must be a node existing in tree.
static void tree_node_remove_impl(tree_t *rbt, tree_node_t *z) {
  tree_node_t *nil = rbt->nil;
  tree_node_t *x, *y;
  y = z;
  int y_was_red = y->red;

  if (z->left == nil) {
    x = z->right;
    tree_node_replace(rbt, z, z->right);
  } else if (z->right == nil) {
    x = z->left;
    tree_node_replace(rbt, z, z->left);
  } else {
    // find tree successor with a null node as a child
    y = tree_node_leftmost(rbt, z->right);
    y_was_red = y->red;
    x = y->right;
    if (y->parent == z) x->parent = y;
    else {
      tree_node_replace(rbt, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    tree_node_replace(rbt, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->red = z->red;
  }

  if (!y_was_red) {
    tree_node_remove_balance(rbt, x);
  }

  rbt->free(z);
}

// Returns 0 on success, -1 on failure (when key is not in the tree).
static int tree_node_remove(tree_t *tree, tree_node_t *node, void *key) {
  node = tree_node_find(tree, node, key);
  if (node != tree->nil) {
    tree_node_remove_impl(tree, node);
    tree->count--;
    return 0;
  }
  return -1;
}

static void tree_node_destroy(tree_t *tree, tree_node_t *node) {
  if (node != tree->nil) {
    tree_node_destroy(tree, node->left);
    tree_node_t *right = node->right;
    tree->free(node);
    tree_node_destroy(tree, right);
  }
}

void tree_destroy(tree_t *tree) {
  if (tree) {
    tree_node_destroy(tree, tree->root);
    tree->free(tree->nil);
    tree->root = tree->nil = (tree_node_t *)0;
  }
}

int tree_remove(tree_t *tree, void *key) {
  if (key) {
    return tree_node_remove(tree, tree->root, key);
  }
  return -1;
}

// k1, k2 must be in the tree.
static inline void swap_key(tree_t *tree, void *k1, void *k2) {
  size_t sz = tree->element_size;
  void *t = tree_node_get_key(tree->nil);
  memcpy(t, k1, sz);
  memcpy(k1, k2, sz);
  memcpy(k2, t, sz);
}

void tree_update(tree_t *tree, void *key) {
  void *next = tree_next_key(tree, key);
  while (next && tree->cmp(key, next) > 0) {
    swap_key(tree, key, next);
    key = next;
    next = tree_next_key(tree, next);
  }
  void *last = tree_prev_key(tree, key);
  while (last && tree->cmp(key, last) < 0) {
    swap_key(tree, key, last);
    key = last;
    last = tree_prev_key(tree, last);
  }
}
