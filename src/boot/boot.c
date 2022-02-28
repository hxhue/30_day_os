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

boot_info_t g_boot_info;

TSS32_t g_tss_sys, g_tss_b;
void *task_b_esp;

// Temporary
layer_t *window_layer1;
layer_t *window_layer2;

// Temporary
static void window_random_square() {
  draw_rect(window_layer1, xrand() % RGB_TRANSPARENT, 1, 1, 32, 32);
  emit_redraw_event(window_layer1->x + 1, window_layer1->y + 1, 
                    window_layer1->x + 32, window_layer1->y + 32);
  add_timer(20, window_random_square);
}

static void switch_to_task_b() {
  xprintf("[switch_to_task_b]\n");
  asm_farjmp(0, 4 * 8);
  add_timer(100, switch_to_task_b);
}

void task_b_main() {
  window_layer2 = make_window(160, 52, "Inputbox2");
  layer_move_to(window_layer2, 260, 130);
  draw_textbox(window_layer2, 8, 28, 144, 16, RGB_WHITE);
  layer_set_rank(window_layer2, 2);
  u64 count = g_counter.count + 36;
  for (;;) {
    xprintf("%d ", (int)g_counter.count);
    if (g_counter.count > count) {
      xprintf("\n");
      asm_farjmp(0, 3 * 8);
      // count needs to be updated
      count = g_counter.count + 36;
    }
    draw_rect(window_layer2, xrand() % 16, 0, 0, 32, 32);
    layers_redraw_all(window_layer2->x, window_layer2->y, window_layer2->x + 32, window_layer2->y + 32);
    asm_hlt();
  }
}

void startup(void) {
  g_boot_info = *(boot_info_t *)0x0ff0;
  init_gdt();
  init_interrupt();
  init_mem_mgr();

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
  layer_set_rank(window_layer1, 2);
  g_tss_sys.ldtr = 0;
  g_tss_sys.iomap = 0x40000000;
  g_tss_b.ldtr = 0;
  g_tss_b.iomap = 0x40000000;
  asm_load_tr(3 * 8);
	task_b_esp = alloc_mem_4k(64 * 1024) + 64 * 1024;
	g_tss_b.eip = (int) &task_b_main;
	g_tss_b.eflags = 0x00000202; /* IF = 1; */
	g_tss_b.eax = 0;
	g_tss_b.ecx = 0;
	g_tss_b.edx = 0;
	g_tss_b.ebx = 0;
	g_tss_b.esp = (int)task_b_esp;
	g_tss_b.ebp = 0;
	g_tss_b.esi = 0;
	g_tss_b.edi = 0;
	g_tss_b.es = 1 * 8;
	g_tss_b.cs = 2 * 8;
	g_tss_b.ss = 1 * 8;
	g_tss_b.ds = 1 * 8;
	g_tss_b.fs = 1 * 8;
	g_tss_b.gs = 1 * 8;
  add_timer(20, window_random_square);
  add_timer(100, switch_to_task_b);

  event_loop();

  xassert(!"Unreachable");
}
