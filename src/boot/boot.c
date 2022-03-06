#include "graphics/draw.h"
#include "event/mouse.h"
#include "support/queue.h"
#include "task/task.h"
#include "support/asm.h"
#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <memory/memory.h>
#include <support/debug.h>
#include <support/priority_queue.h>
#include <support/asm.h>
#include "graphics/layer.h"
#include "event/timer.h"
#include "support/debug.h"
#include <stdlib.h>

boot_info_t g_boot_info;

// Temporary
layer_t *window_layer1;
layer_t *window_layer2;

// Temporary
// static void window_random_square() {
//   draw_rect(window_layer1, rand() % RGB_TRANSPARENT, 1, 1, 32, 32);
//   emit_draw_event(window_layer1->x + 1, window_layer1->y + 1, 
//                   window_layer1->x + 32, window_layer1->y + 32, 0);
//   add_timer(20, window_random_square);
// }

void task_b_main() {
  window_layer2 = make_window(16 + 640, 28 + 9 + 384, "Console");
  layer_move_to(window_layer2, 260, 130);
  draw_textbox(window_layer2, 8, 28, 640, 384, RGB_BLACK);
  layer_bring_to_front(window_layer2);

  // Register mouse event
  process_register_event(current_proc_node, EVENTNO_MOUSE);
  int drag_mode = 0;
  decoded_mouse_msg_t last_msg = {0};

  for (;;) {
    // Check events
    process_t *proc = get_proc_from_node(current_proc_node);
    if (proc->events & EVENTBIT_MOUSE) {
      proc->events &= ~EVENTBIT_MOUSE;

      queue_t *q = &proc->mouse_msg_queue;
      while (!queue_is_empty(q)) {
        decoded_mouse_msg_t msg;
        queue_pop(q, &msg);

        int in_region = 0;
        if (msg.button[0] && msg.layer) {
          int x0 = msg.layer->x; 
          int y0 = msg.layer->y; 
          int x1 = x0 + msg.layer->width;
          int y1 = y0 + msg.layer->height;
          in_region = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y1;
        }
        if (!last_msg.button[0] && msg.button[0] && in_region) {
          drag_mode = 1;
        } else if (!msg.button[0]) {
          drag_mode = 0;
        }

        if (drag_mode) {
          layer_move_by(msg.layer, msg.mx, msg.my);
          layer_bring_to_front(msg.layer);
        }

        last_msg = msg;
      }
    }
    asm_hlt();
  }
}

void task_c_main() {
  window_layer1 = make_window(160, 52, "Inputbox");
  layer_move_to(window_layer1, 160, 100);
  draw_textbox(window_layer1, 8, 28, 144, 16, RGB_WHITE);
  layer_bring_to_front(window_layer1);

  // Register mouse event
  process_register_event(current_proc_node, EVENTNO_MOUSE);
  int drag_mode = 0;
  decoded_mouse_msg_t last_msg = {0};

  for (;;) {
    // Check events
    process_t *proc = get_proc_from_node(current_proc_node);
    if (proc->events & EVENTBIT_MOUSE) {
      proc->events &= ~EVENTBIT_MOUSE;

      queue_t *q = &proc->mouse_msg_queue;
      while (!queue_is_empty(q)) {
        decoded_mouse_msg_t msg;
        queue_pop(q, &msg);

        int in_region = 0;
        if (msg.button[0] && msg.layer) {
          int x0 = msg.layer->x; 
          int y0 = msg.layer->y; 
          int x1 = x0 + msg.layer->width;
          int y1 = y0 + msg.layer->height;
          in_region = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y1;
        }
        if (!last_msg.button[0] && msg.button[0] && in_region) {
          drag_mode = 1;
        } else if (!msg.button[0]) {
          drag_mode = 0;
        }

        if (drag_mode) {
          layer_move_by(msg.layer, msg.mx, msg.my);
          layer_bring_to_front(msg.layer);
        }

        last_msg = msg;
      }
    }

    asm_hlt();
  }
}

void startup(void) {
  g_boot_info = *(boot_info_t *)0x0ff0;

  xprintf("g_boot_info: width:%d, height:%d\n", g_boot_info.width,
          g_boot_info.height);

  init_gdt();
  init_interrupt();
  init_mem_mgr();
  init_task_mgr();

  // Initialize event loop data, e.g. event queue.
  prepare_event_loop();

  /*
   * Init devices and allow some external interrupts.
   * Once devices are initialized, events will come, so event queue must be
   * initialized before that.
   */
  init_devices();

  // Set color and cursor; draw a background.
  init_display();
  
  void *task_b_esp = (char *)alloc_mem_4k(64 * 1024) + 64 * 1024;
  process_t *pb = process_new(6, "InputBox2");
  pb->tss.eip = (int)&task_b_main;
  pb->tss.esp = (int)task_b_esp;
  pb->tss.es = 1 * 8;
  pb->tss.cs = 2 * 8;
  pb->tss.ss = 1 * 8;
  pb->tss.ds = 1 * 8;
  pb->tss.fs = 1 * 8;
  pb->tss.gs = 1 * 8;
  process_start(pb);

  void *task_c_esp = (char *)alloc_mem_4k(64 * 1024) + 64 * 1024;
  process_t *pc = process_new(6, "InputBox");
  pc->tss.eip = (int)&task_c_main;
  pc->tss.esp = (int)task_c_esp;
  pc->tss.es = 1 * 8;
  pc->tss.cs = 2 * 8;
  pc->tss.ss = 1 * 8;
  pc->tss.ds = 1 * 8;
  pc->tss.fs = 1 * 8;
  pc->tss.gs = 1 * 8;
  process_start(pc);

  event_loop();

  xassert(!"Unreachable");
}
