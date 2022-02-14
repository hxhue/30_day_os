#include <stdio.h>
#include "os/inst.h"
#include "os/draw.h"
#include "os/boot.h"

struct boot_info_t g_boot_info;

void OS_startup(void) {
  /* Copy boot information, so we won't need dereference a pointer every time. */
  g_boot_info = *(struct boot_info_t *)0x0ff0;
  init_screen();

  put_char(RGB_WHITE, 0, 0, 'A');
  put_string(RGB_AQUA, 24, 0, "ABC  Hello, world!");
  put_string(RGB_RED_DARK, 0, 16, "Bootstrap main");

  char buf[2048];
  sprintf(buf, "%s%d", "Day", 3);
  put_string(RGB_GREEN, 0, 32, buf);

  put_image((const char *)g_cursor, 16, 16, 160, 100);

  for (;;) {
    asm_hlt();
  }
}

