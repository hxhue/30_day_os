#include <assert.h>
#include <boot/boot_info.h>
#include <graphics/draw.h>
#include <inst.h>
#include <stdio.h>
#include <string.h>

void handle_assertion_failure(const char *assertion, const char *file, int line) {
  fill_rect(RGB_BLACK, 0, g_boot_info.height - 16, g_boot_info.width,
            g_boot_info.height);
  char buf[128];
  sprintf(buf, "ASF:%s:%d: %-.100s", file, line, assertion);
  put_string(RGB_RED, 0, g_boot_info.height - 16, buf);
  for (;;) {
    asm_hlt();
  }
}
