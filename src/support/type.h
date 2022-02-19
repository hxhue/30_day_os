#ifndef TYPE_H
#define TYPE_H

#if (defined(__cplusplus))
extern "C" {
#endif

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

// Returns y in range [low, high].
// If low > high, there is no garantee the result will be correct.
static inline i32 clamp_i32(i32 x, i32 low, i32 high) {
  if (x < low)   return low;
  if (x > high)  return high;
  return x;
}

static inline void swap8(u8 *x, u8 *y) {
  u8 t = *x;
  *x = *y;
  *y = t;
}

static inline void swap16(u16 *x, u16 *y) {
  u16 t = *x;
  *x = *y;
  *y = t;
}

static inline void swap32(u32 *x, u32 *y) {
  u32 t = *x;
  *x = *y;
  *y = t;
}

static inline void swap64(u64 *x, u64 *y) {
  u64 t = *x;
  *x = *y;
  *y = t;
}

static inline u32 next_power_of2(u32 n) {
  --n;

  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;

  return n + 1;
}

#if (defined(__cplusplus))
}
#endif

#endif
