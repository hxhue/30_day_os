#include "graphics/draw.h"
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
static void window_random_square() {
  draw_rect(window_layer1, rand() % RGB_TRANSPARENT, 1, 1, 32, 32);
  emit_redraw_event(window_layer1->x + 1, window_layer1->y + 1, 
                    window_layer1->x + 32, window_layer1->y + 32);
  add_timer(20, window_random_square);
}

void task_b_main() {
  window_layer2 = make_window(160, 52, "Inputbox2");
  layer_move_to(window_layer2, 260, 130);
  draw_textbox(window_layer2, 8, 28, 144, 16, RGB_WHITE);
  layer_bring_to_front(window_layer2);

  // Register mouse IRQ
  process_register_irq(current_proc_node, IRQNO_MOUSE);

  for (;;) {
    draw_rect(window_layer2, rand() % 16, 0, 0, 32, 32);
    layers_redraw_all(window_layer2->x, window_layer2->y, window_layer2->x + 32,
                      window_layer2->y + 32);
    process_t *proc = get_proc_from_node(current_proc_node);
    if (proc->irq & IRQBIT_MOUSE) {
      xprintf("Task b receives a mouse event!\n");
      proc->irq &= ~IRQBIT_MOUSE;
    }
    asm_hlt();
  }
}

void startup(void) {
  g_boot_info = *(boot_info_t *)0x0ff0;
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

  window_layer1 = make_window(160, 52, "Inputbox");
  layer_move_to(window_layer1, 160, 100);
  draw_textbox(window_layer1, 8, 28, 144, 16, RGB_WHITE);
  layer_bring_to_front(window_layer1);
  
  void *task_b_esp = alloc_mem_4k(64 * 1024) + 64 * 1024;
  process_t *pb = process_new(6, "Random Squaure");
  pb->tss.eip = (int)&task_b_main;
  pb->tss.esp = (int)task_b_esp;
  pb->tss.es = 1 * 8;
  pb->tss.cs = 2 * 8;
  pb->tss.ss = 1 * 8;
  pb->tss.ds = 1 * 8;
  pb->tss.fs = 1 * 8;
  pb->tss.gs = 1 * 8;
  process_start(pb);

  add_timer(20, window_random_square);

  event_loop();

  xassert(!"Unreachable");
}
