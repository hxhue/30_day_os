#include <assert.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <inst.h>
#include <stdio.h>
#include <boot/boot_info.h>
#include <type.h>

typedef struct event_queue_t {
  event_t *queue;
  u32 front, end, capacity;
} event_queue_t;

void handle_event_kbd(int data);
void handle_event_mouse(int data);
void handle_event_redraw(int data); // Data is not used

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
    [EVENT_REDRAW]   = handle_event_redraw,
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

void handle_event_kbd(int keycode) {
  char buf[8];
  sprintf(buf, "0X%02X", keycode);
  fill_rect(RGB_AQUA_DARK, 0, 0, 320, 16);
  put_string(RGB_WHITE, 0, 0, buf);
}

typedef struct mouse_msg_t {
  u8 buf[3];
} mouse_msg_t;

static void inline handle_event_mouse_impl(mouse_msg_t msg) {
  int btn = msg.buf[0] & 0x07;
  // Since bitwise operations below assumes integers are 32 bits, we use i32 to
  // ensure that. "mouse_x" and "mouse_y" mean the offset of mouse movement.
  i32 mouse_x = msg.buf[1];
  i32 mouse_y = msg.buf[2];

  if (msg.buf[0] & 0x10)
    mouse_x |= 0xffffff00;
  if (msg.buf[0] & 0x20)
    mouse_y |= 0xffffff00;
  
  // Direction of Y-axis of mouse is opposite to that of the screen.
  mouse_y = -mouse_y;

  // fill_rect(RGB_BLACK, 0, 0, g_boot_info.width, 16);
  // char buf[64];
  // sprintf(buf, "___ x:%d, y:%d", mouse_x, mouse_y);

  // if (btn & 0x01) buf[0] = 'L'; // Left button down
  // if (btn & 0x02) buf[1] = 'R'; // Center button down
  // if (btn & 0x04) buf[2] = 'C'; // Right button down

  // put_string(RGB_WHITE, 0, 0, buf);

  // Move cursor
  g_cursor_stat.x = clamp_i32(g_cursor_stat.x + mouse_x, 0, g_boot_info.width - 1);
  g_cursor_stat.y = clamp_i32(g_cursor_stat.y + mouse_y, 0, g_boot_info.height - 1);

  // Redraw window
  if (mouse_x || mouse_y) {
    raise_event((event_t){.type = EVENT_REDRAW, .data = 0});
  }
}

void handle_event_mouse(int data) {
  // Every mouse event has 3 bytes. Since the port is 8 bit, a mouse event will
  // cause 3 interrupts.

  // The first byte a mouse will receive is 0xfa meaning that mouse
  // initialization is ready.
  static int mouse_state = 3;
  static mouse_msg_t msg = {{0}};

  switch(mouse_state) {
    case 0:
      // Move bits:  0x0~0x3
      // Click bits: 0x8~0xf
      if ((data & 0xc8) == 0x08)
        msg.buf[mouse_state++] = data;
      break;
    case 1:
      msg.buf[mouse_state++] = data;
      break;
    case 2:
      msg.buf[2] = data;
      mouse_state = 0;
      handle_event_mouse_impl(msg);
      break;
    case 3:
      if (data == 0xfa)
        mouse_state = 0;
      break;
    default:
      assert(!"Unreachable");
      break;
  }
}

void handle_event_redraw(int data_used) {
  window_redraw_all();
}

