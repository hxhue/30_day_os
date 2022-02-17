#include <assert.h>
#include <boot/boot_info.h>
#include <boot/descriptor_table.h>
#include <boot/int.h>
#include <graphics/draw.h>
#include <inst.h>
#include <stdio.h>
#include <type.h>
#include <event/event.h>

boot_info_t g_boot_info;

void OS_startup(void) {
  /* Copy boot info, so we won't need to dereference a pointer later on. */
  g_boot_info = *(boot_info_t *)0x0ff0;

  init_descriptor_tables();

  /* Initialize PIC. Do not allow external interrupts. */
  init_pic();

  /* Allow interrupts. External interrupts are not allowed yet. */
  asm_sti();

  // Set color and cursor; draw a background.
  init_screen();

  put_char(RGB_WHITE, 0, 0, 'A');
  put_string(RGB_AQUA, 24, 0, "ABC  Hello, world!");
  put_string(RGB_RED_DARK, 0, 16, "Bootstrap main");
  char buf[2048];
  sprintf(buf, "sizeof(gdt_item):%d", sizeof(segment_descriptor_t));
  put_string(RGB_GREEN, 0, 32, buf);
  put_image((u8 *)g_cursor, 16, 16, 160, 100);

  // Initialize event loop data, e.g. event queue.
  prepare_event_loop();

  // Init devices and allow some external interrupts.
  // Once devices are initialized, events will come, so event queue
  // must be initialized before that.
  init_devices();

  event_loop();
  
  assert(!"Unreachable");
}
