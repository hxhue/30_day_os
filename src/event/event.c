#include <assert.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <inst.h>
#include <stdio.h>
#include <boot/boot_info.h>

typedef struct event_queue_t {
  event_t *queue;
  u32 front, end, capacity;
} event_queue_t;

void handle_event_kbd(int data);
void handle_event_mouse(int data);

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
  // Ignore event when queue is full
  if (next != q->front) {
    q->queue[q->end] = e;
    q->end = next;
  }
}

static void (*event_handler_vec[NUM_EVENT_TYPES])(int) = {
    [EVENT_KEYBOARD] = handle_event_kbd,
    [EVENT_MOUSE]    = handle_event_mouse,
};

static inline void init_event_queue(event_queue_t *q) {
  static event_t event_buf[4096];
  // Only allow initializing global event queue now
  assert(q == &g_event_queue);
  q->queue = event_buf;
  // front == end                  : empty
  // (end + 1) % capacity == front : full
  q->front = 0;
  q->end = 0;
  q->capacity = sizeof(event_buf) / sizeof(event_t);
}

void raise_event(event_t e) {
  push_event(&g_event_queue, e);
}

void prepare_event_loop() {
  init_event_queue(&g_event_queue);
}

void event_loop() {
  for (;;) {
    while (!event_queue_is_empty(&g_event_queue)) {
      event_t e = pop_event(&g_event_queue);
      event_handler_vec[e.type](e.data);
    }
    asm_hlt();
  }
}

void handle_event_kbd(int keycode) {
  char buf[8];
  sprintf(buf, "0X%02X", keycode);
  fill_rect(RGB_AQUA_DARK, 0, 0, 320, 16);
  put_string(RGB_WHITE, 0, 0, buf);
}

void handle_event_mouse(int data) {
  static int counter = 0;
  fill_rect(RGB_BLACK, 0, 0, g_boot_info.width, 16);
  char buf[64];
  sprintf(buf, "INT 2C (IRQ-12): %d", counter++);
  put_string(RGB_WHITE, 0, 0, buf);
}
