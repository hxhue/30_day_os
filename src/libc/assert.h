#ifndef ASSERT_H
#define ASSERT_H

#if (defined(__cplusplus))
extern "C" {
#endif

void handle_assertion_failure(const char *assertion, const char *file, int line);

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define TO_STRING(s) TO_STRING_IMPL(s)
#define TO_STRING_IMPL(s) #s

#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      handle_assertion_failure(TO_STRING(condition), __FILE__, __LINE__);      \
  } while (0)
  
#endif

#if (defined(__cplusplus))
}
#endif

#endif
