#include <support/skiplist.h>
#include <support/xlibc.h>

#define MAX_SKIPLIST_HEIGHT 8

struct sl_entry {
    char * key;
    char * value;
    int height;
    struct sl_entry * next[MAX_SKIPLIST_HEIGHT];
};

// Returns a random number in the range [1, max] following the geometric 
// distribution.
static inline int georand(int max) {
  xassert(max > 1);
  int result = 1;
  while (result < max && (xrand() > XRAND_MAX / 2)) {
    ++result;
  }
  return result;
}


