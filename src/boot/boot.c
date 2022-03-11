#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/mouse.h>
#include <event/timer.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stdlib.h>
#include <support/asm.h>
#include <support/debug.h>
#include <support/priority_queue.h>
#include <support/queue.h>
#include <task/task.h>
#include <app/console.h>

boot_info_t g_boot_info;

void task_c_main() {
  layer_t *window_layer1 = window_layer1 = make_window(160, 52, "Inputbox");
  layer_move_to(window_layer1, 160, 100);
  draw_textbox(window_layer1, 8, 28, 144, 16, RGB_WHITE);
  layer_bring_to_front(window_layer1);

  // Register mouse event
  process_event_listen(current_proc_node, EVENTNO_MOUSE);
  decoded_mouse_msg_t last_msg = {0};
  int drag_mode = 0;
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
    snprintf(buf, 64, "%d", draw_queue_size());
    draw_string(window_layer1, RGB_BLACK, 16, 28, buf, 1);
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

  void *console_esp = (char *)alloc_mem_4k(64 * 1024) + 64 * 1024;
  process_t *pcon = process_new(6, "Console");
  pcon->tss.eip = (int)&console_main;
  pcon->tss.esp = (int)console_esp;
  pcon->tss.es = 1 * 8;
  pcon->tss.cs = 2 * 8;
  pcon->tss.ss = 1 * 8;
  pcon->tss.ds = 1 * 8;
  pcon->tss.fs = 1 * 8;
  pcon->tss.gs = 1 * 8;
  process_start(pcon);

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
