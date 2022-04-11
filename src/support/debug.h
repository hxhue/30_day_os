#if (!defined(XLIBC_H))
#define XLIBC_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <stdio.h>
#include <support/asm.h>

#ifdef NDEBUG

  #define xassert(condition)   ((void)0)
  #define xprintf(format, ...) ((void)0)

#else
  #include <stdio.h>
  #include <support/asm.h>
  #include <support/debug.h>

  static inline int debug_write_console(const char *s) {
    int n = 0;
    for (; *s; ++s) {
      asm_out8(0x3F8, *s); // Qemu serial port
      ++n;
    }
    return n;
  }

  #define TO_STRING(s) TO_STRING_IMPL(s)
  #define TO_STRING_IMPL(s) #s
  #define xassert(condition)                                                   \
    do {                                                                       \
      if (!(condition)) {                                                      \
        char buf[128];                                                         \
        snprintf(buf, sizeof(buf), "\nAssertion failure:\n\t%s:%d: %-.100s\n", \
                __FILE__, __LINE__, TO_STRING(condition));                     \
        const char *p;                                                         \
        for (p = buf; *p; ++p) {                                               \
          asm_out8(0x3F8, *p);                                                 \
        }                                                                      \
        for (;;) {                                                             \
          asm_hlt();                                                           \
        }                                                                      \
      }                                                                        \
    } while (0)

#define xprintf(fmt, ...)                                                      \
  do {                                                                         \
    char buf[2048];                                                            \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__);                            \
    debug_write_console(buf);                                                  \
  } while (0)

#endif

#if (defined(__cplusplus))
        }
#endif

#endif
