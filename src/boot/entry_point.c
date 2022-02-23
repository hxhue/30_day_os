#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <memory/memory.h>
#include <support/debug.h>
#include <support/priority_queue.h>

boot_info_t g_boot_info;

void check_boot_info();

void OS_startup(void) {
  g_boot_info = *(boot_info_t *)0x0ff0;

  check_boot_info();

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

void check_boot_info() {
  xprintf("g_boot_info:\n");
  xprintf("\twidth: %d, height: %d\n", g_boot_info.width, g_boot_info.height);
  xprintf("\tstart of vram: 0X%08X\n", g_boot_info.vram_addr);

  // vbe_mode_info is only temporarily valid and not saved for further usages.
  vbe_mode_info_t *vbe_info = (vbe_mode_info_t *)(0x9000 * 16 + 0);
  xprintf("VBE info from BIOS initialization:\n");
  xprintf("\twidth: %d, height: %d\n", vbe_info->width, vbe_info->height);
  xprintf("\tstart of vram: 0X%08X\n", vbe_info->framebuffer);
  xprintf("\tbits per pixel: %d\n", vbe_info->bpp);
}
