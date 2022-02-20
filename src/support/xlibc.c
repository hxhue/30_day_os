#include <stdio.h>
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

int xputs( const char *p ) {
  int n = 0;
  for (; *p; ++p) {
    asm_out8(0x3F8, *p); // Qemu serial port
    ++n;
  }
  asm_out8(0x3F8, '\n');
  return n + 1;
}
