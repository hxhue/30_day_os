#if (!defined(XLIBC_H))
#define XLIBC_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#define XRAND_MAX 32767

int  xrand();
void xsrand( unsigned seed );

void handle_assertion_failure(const char *assertion, const char *file, int line);

#ifdef NDEBUG
#define xassert(condition) ((void)0)
#else
#define TO_STRING(s) TO_STRING_IMPL(s)
#define TO_STRING_IMPL(s) #s

#define xassert(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      handle_assertion_failure(TO_STRING(condition), __FILE__, __LINE__);      \
  } while (0)
  
#endif

#if (defined(__cplusplus))
	}
#endif

#endif
