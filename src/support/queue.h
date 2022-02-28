#ifndef QUEUE_H
#define QUEUE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <support/debug.h>
#include <memory/memory.h>
#include <string.h>

typedef struct queue_t {
  u8 *queue;
  size_t front, end, capacity, element_size;
  malloc_fp_t alloc;
  free_fp_t free;
} queue_t;

static inline unsigned int queue_size(const queue_t *q) {
  return (q->end - q->front + q->capacity) % q->capacity;
}

static inline int queue_is_empty(const queue_t *q) {
  return q->front == q->end;
}

static inline void queue_init(queue_t *q, size_t element_size,
                              size_t capacity, malloc_fp_t alloc,
                              free_fp_t free) {
  xassert(q && alloc && free);
  q->element_size = element_size;
  q->capacity = capacity;
  q->front = 0;
  q->end = 0;
  q->alloc = alloc;
  q->free = free;
  q->queue = q->alloc(element_size * capacity);
}

static inline void queue_destroy(queue_t *q) {
  q->front = q->end;
  q->free(q->queue);
  q->queue = 0;
}

static inline void queue_pop(queue_t *q, void *out) {
  xassert(!queue_is_empty(q));
  memcpy(out, q->queue + q->front * q->element_size, q->element_size);
  if (++q->front >= q->capacity) {
    q->front -= q->capacity;
  }
}

// Get the first element, but do not remove it.
static inline void queue_peek(queue_t *q, void *out) {
  xassert(!queue_is_empty(q));
  memcpy(out, q->queue + q->front * q->element_size, q->element_size);
}

// Get the pointer to the first element.
static inline void *queue_get_first(queue_t *q) {
  xassert(!queue_is_empty(q));
  return q->queue + q->front * q->element_size;
}

// Returns 0 on success, -1 on failure.
static inline int queue_push(queue_t *q, const void *in) {
  u32 next = (q->end + 1) % q->capacity;
  if (next != q->front) {
    memcpy(q->queue + q->end * q->element_size, in, q->element_size);
    q->end = next;
    return 0;
  }
  xprintf("Warning: queue_push() on 0X%p failed because queue is full\n", q);
  return -1;
}

// Returns 0 on success, -1 on failure.
static inline int queue_push_no_warning(queue_t *q, const void *in) {
  u32 next = (q->end + 1) % q->capacity;
  if (next != q->front) {
    memcpy(q->queue + q->end * q->element_size, in, q->element_size);
    q->end = next;
    return 0;
  }
  return -1;
}

static inline void queue_clear(queue_t *q) {
  q->front = q->end = 0;
}

#if (defined(__cplusplus))
}
#endif

#endif

