#if (!defined(XLIBC_H))
#define XLIBC_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <stdio.h>
#include <support/asm.h>

#define XRAND_MAX 32767

int  xrand();
void xsrand(unsigned seed);

void debug_xassert_failure_impl(const char *assertion, const char *file, int line);
int  debug_write_console(const char *s);

#ifdef NDEBUG

#define xassert(condition)   ((void)0)
#define xprintf(format, ...) ((void)0)
#define xputs(str)           ((void)0)

#else

#define TO_STRING(s) TO_STRING_IMPL(s)
#define TO_STRING_IMPL(s) #s
#define xassert(condition)                                                     \
  do {                                                                         \
    if (!(condition))                                                          \
      debug_xassert_failure_impl(TO_STRING(condition), __FILE__, __LINE__);    \
  } while (0)
static inline int xputs(const char *str) {
  int n = debug_write_console(str);
  asm_out8(0x3F8, '\n');
  return n + 1;
}
#define xprintf(fmt, ...)                                                      \
  do {                                                                         \
    char xprintf_buf[512];                                                     \
    sprintf(xprintf_buf, fmt, ##__VA_ARGS__);                                  \
    debug_write_console(xprintf_buf);                                          \
  } while (0)
  
#endif
#if (defined(__cplusplus))
        }
#endif

#endif
