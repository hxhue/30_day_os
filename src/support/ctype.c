#include <ctype.h>

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}

int isalpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isdigit(int c) { return c >= '0' && c <= '9'; }

int isspace(int c) {
  switch (c) {
  case ' ':
  case '\f':
  case '\n':
  case '\r':
  case '\t':
  case '\v':
    return 1;
  default:
    break;
  }
  return 0;
}

int isblank(int c) {
  return c == ' ' || c == '\t';
}