#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <memory/memory.h>
#include <support/debug.h>
#include <support/priority_queue.h>

boot_info_t g_boot_info;

void fix_vbe_info();

void OS_startup(void) {
  g_boot_info = *(boot_info_t *)0x0ff0;
  fix_vbe_info();

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

  event_loop();

  xassert(!"Unreachable");
}

// int integer_less(void *a, void *b) {
//   return *(int *)a < *(int *)b;
// }

// void integer_swap(void *a, void *b) {
//   int t = *(int *)a;
//   *(int *)a = *(int *)b;
//   *(int *)b = t;
// }

void fix_vbe_info() {
  vbe_mode_info_t *vbe_info = (vbe_mode_info_t *)0x1000;
  xprintf("VBE info:\n");
  xprintf("\twidth: %d, height: %d\n", vbe_info->width, vbe_info->height);
  xprintf("\tstart of vram: 0X%08X\n", vbe_info->framebuffer);
  xprintf("\tbits per pixel: %d\n", vbe_info->bpp);
  g_boot_info.vram_addr = vbe_info->framebuffer;
  g_boot_info.width = vbe_info->width;
  g_boot_info.height = vbe_info->height;
  *(boot_info_t *)0x0ff0 = g_boot_info;
}
