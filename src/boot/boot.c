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

// Temporary
// static void window_random_square() {
//   draw_rect(window_layer1, rand() % RGB_TRANSPARENT, 1, 1, 32, 32);
//   emit_draw_event(window_layer1->x + 1, window_layer1->y + 1, 
//                   window_layer1->x + 32, window_layer1->y + 32, 0);
//   add_timer(20, window_random_square);
// }

void task_console_main() {
  layer_t *window_console = make_window(16 + 640, 28 + 9 + 384, "Console");
  layer_move_to(window_console, 260, 130);
  draw_textbox(window_console, 8, 28, 640, 384, RGB_BLACK);
  layer_bring_to_front(window_console);

  const int TIMER_DATA_CURSOR_BLINK = 1;
  add_timer(500, current_proc_node, TIMER_DATA_CURSOR_BLINK);
  // Register mouse event
  process_event_listen(current_proc_node, EVENTNO_MOUSE);
  int drag_mode = 0;
  int has_focus = 0;
  decoded_mouse_msg_t last_msg = {0};

  for (;;) {
    // Check events
    process_t *proc = get_proc_from_node(current_proc_node);
    if (!queue_is_empty(&proc->mouse_msg_queue)) {
      // proc->events &= ~EVENTBIT_MOUSE;
      queue_t *q = &proc->mouse_msg_queue;
      while (!queue_is_empty(q)) {
        decoded_mouse_msg_t msg;
        queue_pop(q, &msg);

        if (msg.control == MOUSE_EVENT_LOSE_CONTROL) {
          has_focus = 0;
          redraw_window_title(msg.layer, "Console", RGB_GRAY_DARK);
        } else if (msg.button[0] && !has_focus && msg.layer) {
          has_focus = 1;
          redraw_window_title(msg.layer, "Console", RGB_CYAN_DARK);
        }

        int in_title_bar = 0, in_window = 0;
        if (msg.button[0] && msg.layer) {
          int x0 = msg.layer->x; 
          int y0 = msg.layer->y; 
          int x1 = x0 + msg.layer->width;
          int y1 = y0 + msg.layer->height; 
          int y2 = y0 + 21; // Window title bar height: 21
          in_window = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y1;
          in_title_bar = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y2;
        }

        if (!last_msg.button[0] && msg.button[0] && in_window) {
          layer_bring_to_front(msg.layer);
        }

        if (!last_msg.button[0] && msg.button[0] && in_title_bar) {
          drag_mode = 1;
        } else if (!msg.button[0]) {
          drag_mode = 0;
        }

        if (drag_mode) {
          layer_move_by(msg.layer, msg.mx, msg.my);
          // xprintf("Console move by: (%d, %d)\n", msg.mx, msg.my);
        }

        last_msg = msg;
      }
    }

    while (!queue_is_empty(&proc->timer_msg_queue)) {
      int timer_data;
      queue_pop(&proc->timer_msg_queue, &timer_data);
      if (timer_data == TIMER_DATA_CURSOR_BLINK) {
        xprintf("Cursor blink\n\n");
        add_timer(500, current_proc_node, TIMER_DATA_CURSOR_BLINK);
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
  process_event_listen(current_proc_node, EVENTNO_MOUSE);
  int drag_mode = 0;
  decoded_mouse_msg_t last_msg = {0};
  int has_focus = 0;

  for (;;) {
    // Check events
    process_t *proc = get_proc_from_node(current_proc_node);
    if (!queue_is_empty(&proc->mouse_msg_queue)) {
      // proc->events &= ~EVENTBIT_MOUSE;

      queue_t *q = &proc->mouse_msg_queue;
      while (!queue_is_empty(q)) {
        decoded_mouse_msg_t msg;
        queue_pop(q, &msg);
        
        if (msg.control == MOUSE_EVENT_LOSE_CONTROL) {
          // MOUSE_EVENT_LOSE_CONTROL always comes with a layer
          has_focus = 0;
          redraw_window_title(msg.layer, "InpuxBox", RGB_GRAY_DARK);
        } else if (msg.button[0] && !has_focus && msg.layer) {
          has_focus = 1;
          redraw_window_title(msg.layer, "InpuxBox", RGB_CYAN_DARK);
        }

        int in_title_bar = 0, in_window = 0;
        if (msg.button[0] && msg.layer) {
          int x0 = msg.layer->x; 
          int y0 = msg.layer->y; 
          int x1 = x0 + msg.layer->width;
          int y1 = y0 + msg.layer->height; 
          int y2 = y0 + 21; // Window title bar height: 21
          in_window = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y1;
          in_title_bar = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y2;
        }

        if (!last_msg.button[0] && msg.button[0] && in_window) {
          layer_bring_to_front(msg.layer);
        }

        if (!last_msg.button[0] && msg.button[0] && in_title_bar) {
          drag_mode = 1;
        } else if (!msg.button[0]) {
          drag_mode = 0;
        }

        if (drag_mode) {
          layer_move_by(msg.layer, msg.mx, msg.my);
          // xprintf("Console move by: (%d, %d)\n", msg.mx, msg.my);
        }

        last_msg = msg;
      }
    }

    draw_textbox(window_layer1, 8, 28, 144, 16, RGB_WHITE);
    char buf[64];
    extern queue_t draw_msg_queue;
    snprintf(buf, 64, "%d", queue_size(&draw_msg_queue));
    draw_string(window_layer1, RGB_BLACK, 16, 28, buf);
    // xprintf("\r%012d", queue_size(&draw_msg_queue));

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
  pb->tss.eip = (int)&task_console_main;
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
