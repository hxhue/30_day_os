#include <assert.h>
#include <boot/boot_info.h>
#include <boot/descriptor_table.h>
#include <boot/int.h>
#include <graphics/draw.h>
#include <inst.h>
#include <stdio.h>
#include <type.h>

boot_info_t g_boot_info;

void OS_startup(void) {
  /* Copy boot info, so we won't need to dereference a pointer later on. */
  g_boot_info = *(boot_info_t *)0x0ff0;

  init_descriptor_tables();

  /* Initialize PIC. Do not allow external interrupts. */
  init_pic();

  /* Allow interrupts. External interrupts are not allowed yet. */
  asm_sti();

  init_screen();

  put_char(RGB_WHITE, 0, 0, 'A');
  put_string(RGB_AQUA, 24, 0, "ABC  Hello, world!");
  put_string(RGB_RED_DARK, 0, 16, "Bootstrap main");
  char buf[2048];
  sprintf(buf, "sizeof(gdt_item):%d", sizeof(segment_descriptor_t));
  put_string(RGB_GREEN, 0, 32, buf);
  put_image((u8 *)g_cursor, 16, 16, 160, 100);

  /* Allow some external interrupts. */
  asm_out8(PIC0_IMR, 0xf9); /* 0b11111001, keyboard-1 and PIC1-2 */
  asm_out8(PIC1_IMR, 0xef); /* 0b11101111, mouse-12 */
  // asm_out8(PIC0_IMR, 0);
  // asm_out8(PIC1_IMR, 0);

  for (;;) {
    asm_hlt();
  }
}
