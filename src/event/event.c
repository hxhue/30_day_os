#include <memory/memory.h>
#include <boot/boot.h>
#include <boot/def.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/counter.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/queue.h>

#define EVENT_QUEUE_COUNT 3
static event_queue_t event_queues[EVENT_QUEUE_COUNT];

void prepare_event_loop() {
//  init_event_queue(&g_event_queue);
  // queue_init(&g_event_queue, sizeof(event_t), EVENT_QUEUE_SIZE);
  init_mouse_event_queue();
  init_keyboard_event_queue();
  init_redraw_event_queue();
  event_queues[0] = g_mouse_event_queue;
  event_queues[1] = g_kbd_event_queue;
  event_queues[2] = g_redraw_event_queue;
}

_Noreturn void event_loop() {
  for (;;) {
    asm_cli();

    // Search for a non-empty queue.
    int queue_index = -1, i;
    for (i = 0; i < sizeof(event_queues) / sizeof(event_queues[0]); ++i) {
      if (!event_queues[i].empty()) {
        queue_index = i;
        break;
      }
    }

    // No new event.
    if (queue_index < 0) {
      // HLT after STI has a special effect: interrupts between them will stop
      // HLT from making CPU sleep. Calling asm_sti() and asm_hlt() separately
      // does not have this effect.
      // asm_sti_hlt();
      asm_sti();

      extern layer_info_t *window_layer;
      char buf[64];
      sprintf(buf, "%08u", g_timer.count);
      fill_rect(window_layer, RGB_GRAY, 40, 28, 120, 44);
      put_string(window_layer, RGB_BLACK, 40, 28, buf);
      int x = window_layer->x, y = window_layer->y;
      partial_redraw(x + 40, y + 28, x + 120, y + 44);
      continue;
    }

    // asm_sti() is called inside the consume() function.
    event_queues[queue_index].consume();
  }
}

// kbdc is slow so CPU has to wait.
// But kbdc was fast when I tested it in Qemu.
static inline void wait_kbdc_ready() {
  while (asm_in8(PORT_KEYSTA) & KEYSTA_SEND_NOT_READY) {
    // Continue
  }
}

static inline void init_keyboard() {
  wait_kbdc_ready();
  // Send command: set mode (0x60).
  asm_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_kbdc_ready();
  // Send data: a mode that can use mouse (0x47).
  // Mouse communicates through keyboard control circuit.
  asm_out8(PORT_KEYDAT, KBC_MODE);
}

static inline void init_mouse() {
  wait_kbdc_ready();
  // Send command: transfer data to mouse
  asm_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_kbdc_ready();
  // Send data: tell mouse to start working
  asm_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}

static inline void init_counter() {
  // Notify IRQ-0 Cycle change
  asm_out8(PIT_CTRL, 0X34);
  // 0x2e9c == 11932: An interrupt every 10 ms
  // This is calculated by frequency of the clock
  asm_out8(PIT_CNT0, 0x9c);
  asm_out8(PIT_CNT0, 0x2e);
}

// Initialize devices, so they can handle interrupts and emit events.
void init_devices() {
  init_counter();
  asm_out8(PIC0_IMR, 0xf8); /* 0b11111000, PIT:0, keyboard:1 and PIC1:2 */
  asm_out8(PIC1_IMR, 0xef); /* 0b11101111, mouse-12 */
  init_keyboard();
  init_mouse();
}

