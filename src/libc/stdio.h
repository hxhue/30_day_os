/* copyright(C) 2003 H.Kawai (under KL-01). */

#if (!defined(STDIO_H))

#define STDIO_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

// // Bug (in this golibc.lib version): "%08llu" makes program hang.
// int sprintf(char *s, const char *format, ...);
// int vsprintf(char *s, const char *format, va_list arg);

#include <support/printf.h>

#if (defined(__cplusplus))
	}
#endif

#endif
