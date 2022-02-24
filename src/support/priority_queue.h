#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <support/debug.h>
#include <memory/memory.h>
#include <string.h>

// A max-heap priority heap.
typedef struct priority_queue_t {
  int tag;
  // Element 0 is not used; holds (capacity + 1) elements.
  u8 *heap;
  u32 element_size, size, capacity;
  void (*swap)(void *, void *);
  int (*less)(void *, void *);
} priority_queue_t;

static inline u32 priority_queue_size(priority_queue_t *q) {
  return q->size;
}

static inline u32 priority_queue_is_empty(priority_queue_t *q) {
  return q->size == 0;
}

// Requirements: swap != 0, cmp != 0, q != 0
static inline void priority_queue_init(priority_queue_t *q,
                                       unsigned element_size,
                                       unsigned capacity,
                                       void (*swap)(void *, void *),
                                       int (*less)(void *, void *)) {
  q->tag = PRIORITY_QUEUE_STRUCT_TAG;
  q->element_size = element_size;
  q->size = 0;
  q->capacity = capacity;
  q->swap = swap;
  q->less = less;
  q->heap = alloc_mem(element_size * (capacity + 1));
}

static inline void priority_queue_destory(priority_queue_t *q) {
  reclaim_mem(q->heap, q->element_size * (q->capacity + 1));
  q->heap = 0;
  q->size = 0;
}

static inline void priority_queue_swim(priority_queue_t *q, u32 i) {
  u8 *heap = q->heap;
  u32 size = q->element_size;
  while (i > 1) {
    if (q->less(heap + i/2 * size, heap + i * size)) {
      q->swap(heap + i/2 * size, heap + i * size);
      i /= 2;
    } else {
      break;
    }
  }
}

static inline void priority_queue_sink(priority_queue_t *q, u32 i) {
  u8 *heap = q->heap;
  u32 size = q->element_size;
  while (2 * i <= q->size) {
    u32 next = 2 * i;
    if (next + 1 <= q->size && q->less(heap + next * size, heap + (next + 1) * size)) {
      ++next;
    }
    if (q->less(heap + i * size, heap + next * size)) {
      q->swap(heap + i * size, heap + next * size);
      i = next;
    } else {
      break;
    }
  }
}

// Returns 0 on success, -1 on failure.
static inline int priority_queue_push(priority_queue_t *q, const void *in) {
  if (q->size == q->capacity) {
    xprintf("Warning: priority_queue_push() on 0X%p failed "
            "because queue is full\n", q);
    return -1;
  }
  // The first element is skipped. For a heap with size "n", the last elements
  // is at index "n".
  u32 sz = ++q->size;
  memcpy(q->heap + q->element_size * sz, in, q->element_size);
  priority_queue_swim(q, sz);
  return 0;
}

static inline void priority_queue_pop(priority_queue_t *q, void *out) {
  xassert(!priority_queue_is_empty(q));
  u32 sz = q->size--;
  // q->heap[1] is temporarily filled with garbage, but then swapped with the
  // last element. After size is decremented by 1, the heap is normal again.
  q->swap(q->heap + q->element_size, out);
  q->swap(q->heap + q->element_size, q->heap + q->element_size * sz);
  priority_queue_sink(q, 1);
}

// Get the first element, but do not remove it.
static inline void priority_queue_peek(priority_queue_t *q, void *out) {
  xassert(!priority_queue_is_empty(q));
  memcpy(out, q->heap + q->element_size, q->element_size);
}

// Get the pointer to the first element.
static inline void *priority_queue_get_first(priority_queue_t *q) {
  xassert(!priority_queue_is_empty(q));
  return q->heap + q->element_size;
}

#if (defined(__cplusplus))
}
#endif

#endif

