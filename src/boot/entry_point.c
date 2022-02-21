#include <boot/boot.h>
#include <boot/desctbl.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <memory/memory.h>
#include <support/debug.h>

boot_info_t g_boot_info;

void OS_startup(void) {
  /* Copy boot info, so we won't need to dereference a pointer later on. */
  g_boot_info = *(boot_info_t *)0x0ff0;

  init_descriptor_tables();

  // Initialize PIC and allow interrupts. External interrupts are not allowed
  // yet.
  init_pic();

  init_mem_mgr();

  // Initialize event loop data, e.g. event queue.
  prepare_event_loop();

  // Set color and cursor; draw a background.
  init_display();

  // Init devices and allow some external interrupts.
  // Once devices are initialized, events will come, so event queue
  // must be initialized before that.
  init_devices();

  event_loop();

  xassert(!"Unreachable");
}
