#include <boot/boot_info.h>
#include <graphics/draw.h>
#include <stdio.h>
#include <string.h>
#include <support/asm.h>
#include <support/xlibc.h>

static unsigned long rand_seed_next = 1;

int xrand() {
  rand_seed_next = rand_seed_next * 1103515245U + 12345U;
  return (int)((unsigned)rand_seed_next / 65536U % 32768U);
}

void xsrand(unsigned seed) { rand_seed_next = seed; }

void handle_assertion_failure(const char *assertion, const char *file,
                              int line) {
  fill_rect(RGB_BLACK, 0, g_boot_info.height - 16, g_boot_info.width,
            g_boot_info.height);
  char buf[128];
  sprintf(buf, "ASF:%s:%d: %-.100s", file, line, assertion);
  put_string(RGB_RED, 0, g_boot_info.height - 16, buf);
  for (;;) {
    asm_hlt();
  }
}

