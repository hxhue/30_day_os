#include "graphics/draw.h"
#include <memory/memory.h>
#include <boot/boot.h>
#include <boot/def.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/timer.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/queue.h>

static event_queue_t *event_queues[] = {
  &g_mouse_event_queue,
  &g_keyboard_event_queue,
  &g_timer_event_queue,
  &g_redraw_event_queue,
};

void prepare_event_loop() {
  init_mouse_event_queue();
  init_keyboard_event_queue();
  init_redraw_event_queue();
  init_timer_event_queue();
}

void event_loop() {
  for (;;) {
    asm_cli();
    // Search for a non-empty queue.
    int queue_index = -1, i;
    for (i = 0; i < sizeof(event_queues) / sizeof(event_queues[0]); ++i) {
      if (!event_queues[i]->empty()) {
        queue_index = i;
        break;
      }
    }
    // No new event.
    if (queue_index < 0) {
      asm_sti();
      // process_yield();
      continue;
    }
    // asm_sti() is called inside the consume() function.
    event_queues[queue_index]->consume();
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

