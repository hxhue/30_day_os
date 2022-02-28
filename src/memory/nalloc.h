#ifndef NODE_ALLOCATOR_H
#define NODE_ALLOCATOR_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <stddef.h>
#include <support/type.h>

// Lifetime of a node allocator must be longer than nodes it allocates.
struct node_alloc_t {
  unsigned char * mem; // First bmsz bytes are used for bitmap. Bit 1 means free.
  size_t bmsz;
  size_t fast_count;   // [0, bmsz * 8]
  size_t real_size;    // Memory given
  size_t element_size;
};

typedef struct node_alloc_t node_alloc_t;

void node_alloc_init(node_alloc_t *alloc, void *memory, size_t memory_size,
                     size_t element_size);

void *node_alloc_get(node_alloc_t *alloc);

// Returns 0 on success, -1 on failure.
int node_alloc_reclaim(node_alloc_t *alloc, void *addr);

// void node_alloc_wrap(node_alloc_t *alloc, void *memory, size_t memory_size,
//                      size_t element_size, malloc_func_t *wrapped_malloc, 
//                      free_func_t *wrapped_free);

#if (defined(__cplusplus))
}
#endif

#endif

