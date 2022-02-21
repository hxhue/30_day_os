#include <memory/memory.h>
#include <boot/boot.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/xlibc.h>

typedef struct event_queue_t {
  event_t *queue;
  u32 front, end, capacity;
} event_queue_t;

static event_queue_t g_event_queue;

static inline int event_queue_is_empty(const event_queue_t *q) {
  return q->front == q->end;
}

static inline int event_queue_size(const event_queue_t *q) {
  return (q->end - q->front + q->capacity) % q->capacity;
}

static inline event_t pop_event(event_queue_t *q) {
  event_t e = q->queue[q->front];
  if (++q->front >= q->capacity)
    q->front -= q->capacity;
  return e;
}

static inline void push_event(event_queue_t *q, event_t e) {
  u32 next = (q->end + 1) % q->capacity;
  
  if (next != q->front) {
    q->queue[q->end] = e;
    q->end = next;
  } else {
    // Ignore event when queue is full
    xprintf("Warning: Event{%02d, 0X%08X} is discarded because queue is full\n",
            e.type, e.data);
  }
}

static void (*event_handler_vec[NUM_EVENT_TYPES])(int) = {
    [EVENT_KEYBOARD] = handle_event_kbd,
    [EVENT_MOUSE]    = handle_event_mouse,
    [EVENT_REDRAW]   = handle_event_redraw,
};

#define EVENT_QUEUE_SIZE 4096

static inline void init_event_queue(event_queue_t *q) {
  // Only allow initializing global event queue now
  xassert(q == &g_event_queue);
  q->queue = alloc_mem_4k(EVENT_QUEUE_SIZE * sizeof(event_t));
  // front == end                  : empty
  // (end + 1) % capacity == front : full
  q->front = 0;
  q->end = 0;
  q->capacity = EVENT_QUEUE_SIZE;
  xassert(q->queue);
}

void raise_event(event_t e) {
  push_event(&g_event_queue, e);
}

void prepare_event_loop() {
  init_event_queue(&g_event_queue);
}

void event_loop() {
  for (;;) {
    asm_cli();
    if (event_queue_is_empty(&g_event_queue)) {
      // HLT after STI has a special effect: interrupts bewteen them will stop
      // HLT from making CPU sleep. Calling asm_sti() and asm_hlt() separately
      // does not have this effect.
      asm_sti_hlt();
      continue;
    }
    event_t e = pop_event(&g_event_queue);
    asm_sti();
    event_handler_vec[e.type](e.data);
  }
}

