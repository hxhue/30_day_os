#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <support/type.h>
#include <memory/memory.h>

size_t strlen(const char *s) {
	const char *beg = s;
	while (*s) {
    s++;
  }
	return s - beg;
}

char *strcpy(char *dst, const char *src) {
  size_t n = strlen(src);
  memcpy(dst, src, n);
  dst[n] = '\0';
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t m = strlen(src);
  if (n > m) {
    n = m;
  }
  memcpy(dst, src, n);
  dst[n] = '\0';
	return dst;
}

char *strdup(const char *s) {
  size_t len = strlen(s);
  char *p = alloc(len + 1);
  memcpy(p, s, len);
  p[len] = '\0';
  return p;
}

int strcmp(const char *cs, const char *ct) {
  while (*cs && *ct) {
    if (*cs != *ct)
      break;
    ++cs, ++ct;
  }
  return *cs - *ct;
}

int strncmp(const char *cs, const char *ct, size_t n) {
  const char *end = cs + n;
  while (*cs && *ct && cs < end) {
    if (*cs != *ct)
      break;
    ++cs, ++ct;
  }
  if (cs == end) {
    return 0;
  }
  return *cs - *ct;
}