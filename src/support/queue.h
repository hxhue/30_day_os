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
  int tag;
  u8 *queue;
  u32 front, end, capacity, element_size;
} queue_t;

static inline unsigned int queue_size(const queue_t *q) {
  return (q->end - q->front + q->capacity) % q->capacity;
}

static inline int queue_is_empty(const queue_t *q) {
  return q->front == q->end;
}

static inline void queue_init(queue_t *q, unsigned element_size,
                              unsigned capacity) {
  q->tag = QUEUE_STRUCT_TAG;
  q->element_size = element_size;
  q->capacity = capacity;
  q->front = 0;
  q->end = 0;
  q->queue = alloc_mem_4k(element_size * capacity);
}

static inline void queue_destroy(queue_t *q) {
  q->front = q->end;
  reclaim_mem_4k(q->queue, q->element_size * q->capacity);
  q->queue = 0;
}

static inline void queue_pop(queue_t *q, void *out) {
  xassert(!queue_is_empty(q));
  memcpy(out, q->queue + q->front * q->element_size, q->element_size);
  if (++q->front >= q->capacity) {
    q->front -= q->capacity;
  }
}

static inline void queue_push(queue_t *q, const void *in) {
  u32 next = (q->end + 1) % q->capacity;
  if (next != q->front) {
    memcpy(q->queue + q->end * q->element_size, in, q->element_size);
    q->end = next;
  } else {
    xprintf("Warning: queue_push() on 0X%p failed because queue is full", q);
  }
}

#if (defined(__cplusplus))
}
#endif

#endif

