#ifndef ASSERT_H
#define ASSERT_H

#if (defined(__cplusplus))
extern "C" {
#endif

void handle_assertion_failure(const char *assertion);

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define TO_STRING(s) TO_STRING_IMPL(s)
#define TO_STRING_IMPL(s) #s

#define assert(condition)                                                      \
  do {                                                                         \
    if (!(condition))                                                          \
      handle_assertion_failure(TO_STRING(condition));                          \
  } while (0)

// #undef TO_STRING
// #undef TO_STRING_IMPL
#endif

#if (defined(__cplusplus))
}
#endif

#endif
