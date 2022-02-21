#include <stdio.h>
#include <support/asm.h>
#include <support/xlibc.h>

static unsigned long rand_seed_next = 1;

int xrand() {
  rand_seed_next = rand_seed_next * 1103515245U + 12345U;
  return (int)((unsigned)rand_seed_next / 65536U % 32768U);
}

void xsrand(unsigned seed) { rand_seed_next = seed; }

void debug_xassert_failure_impl(const char *assertion, const char *file,
                              int line) {
  char buf[128];
  sprintf(buf, "\nAssertion failure:\n\t%s:%d: %-.100s\n", file, line, assertion);
  const char *p;
  for (p = buf; *p; ++p) {
    asm_out8(0x3F8, *p); // Qemu serial port
  }
  for (;;) {
    asm_hlt();
  }
}

int debug_write_console(const char *s) {
  int n = 0;
  for (; *s; ++s) {
    asm_out8(0x3F8, *s); // Qemu serial port
    ++n;
  }
  return n;
}
