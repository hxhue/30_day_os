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

  // Initialize event loop data, e.g. event queue.
  prepare_event_loop();

  // Set color and cursor; draw a background.
  init_display();

  // Init devices and allow some external interrupts.
  // Once devices are initialized, events will come, so event queue
  // must be initialized before that.
  init_devices();

  event_loop();
  
  assert(!"Unreachable");
}
