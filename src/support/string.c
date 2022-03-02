#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <support/type.h>

size_t strlen (const char *s) {
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
