#include "graphics/draw.h"
#include "memory/memory.h"
#include "support/tree.h"
#include <memory/memory.h>
#include <boot/boot.h>
#include <boot/def.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/timer.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <stddef.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/queue.h>
#include <support/debug.h>

// Now drawing task is separated.
// static event_queue_t *event_queues[] = {
//   &g_mouse_event_queue,
//   // &g_keyboard_event_queue,
//   &g_timer_event_queue,
// };

void prepare_event_loop() {
  init_mouse_event_queue();
  init_draw_event_queue();
  init_timer_event_queue();
  // init_keyboard_event_queue();
}

// TODO: 把计时器事件放在中断中直接处理。内核为用堆来管理计时器。
//       而中断处理只放置最靠前的一批计时器。

void event_loop() {
  for (;;) {
    // Give up time slices when all event queues are empty.
    if (check_mouse_events() && check_timer_events()) {
      process_yield();
    }
  }
}

// Initialize devices, so they can handle interrupts and emit events.
void init_devices() {
  init_counter();
  asm_out8(PIC0_IMR, 0xf8); /* 0b11111000, PIT:0, keyboard:1 and PIC1:2 */
  asm_out8(PIC1_IMR, 0xef); /* 0b11101111, mouse-12 */
  init_keyboard();
  init_mouse();
}
