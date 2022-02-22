#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <memory/memory.h>
#include <support/debug.h>
#include <support/priority_queue.h>

boot_info_t g_boot_info;

void test();

void OS_startup(void) {
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

  test();

  event_loop();

  xassert(!"Unreachable");
}

int integer_less(void *a, void *b) {
  return *(int *)a < *(int *)b;
}

void integer_swap(void *a, void *b) {
  int t = *(int *)a;
  *(int *)a = *(int *)b;
  *(int *)b = t;
}

void test() {
  priority_queue_t q;
  priority_queue_init(&q, sizeof(int), 64, integer_swap, integer_less);
  int A[] = {32, 4, 1, -7, 2, 3, 4, 77, 23};
  int i = 0, n = sizeof(A)/sizeof(int);
  for (; i < n; ++i) {
    priority_queue_push(&q, &A[i]);
  }
  xprintf("priority_queue: ");
  while (!priority_queue_is_empty(&q)) {
    int a;
    priority_queue_pop(&q, &a);
    xprintf("%d ", a);
  }
  xprintf("\n");
}