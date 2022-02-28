#ifndef TYPE_H
#define TYPE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <stddef.h>

typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef void (*free_fp_t)(void *);
typedef void *(*malloc_fp_t)(size_t);

// Returns y in range [low, high].
// If low > high, there is no garantee the result will be correct.
static inline i32 clamp_i32(i32 x, i32 low, i32 high) {
  if (x < low)   return low;
  if (x > high)  return high;
  return x;
}

static inline u32 clamp_u32(u32 x, u32 low, u32 high) {
  if (x < low)   return low;
  if (x > high)  return high;
  return x;
}

static inline i16 clamp_i16(i16 x, i16 low, i16 high) {
  if (x < low)   return low;
  if (x > high)  return high;
  return x;
}

static inline u16 clamp_u16(u16 x, u16 low, u16 high) {
  if (x < low)   return low;
  if (x > high)  return high;
  return x;
}

static inline i32 min_i32(i32 x, i32 y) {
  return x < y ? x : y;
}

static inline i32 max_i32(i32 x, i32 y) {
  return x > y ? x : y;
}

static inline void swap8(void *x, void *y) {
  u8 *X = (u8 *)x, *Y = (u8 *)y;
  u8 t = *X;
  *X = *Y;
  *Y = t;
}

static inline void swap16(void *x, void *y) {
  u16 *X = (u16 *)x, *Y = (u16 *)y;
  u16 t = *X;
  *X = *Y;
  *Y = t;
}

static inline void swap32(void *x, void *y) {
  u32 *X = (u32 *)x, *Y = (u32 *)y;
  u32 t = *X;
  *X = *Y;
  *Y = t;
}

static inline void swap64(void *x, void *y) {
  u64 *X = (u64 *)x, *Y = (u64 *)y;
  u64 t = *X;
  *X = *Y;
  *Y = t;
}

// Swap for pointers
static inline void swapptr(void *x, void *y) {
  void **X = (void **)x, **Y = (void **)y;
  void *t = *X;
  *X = *Y;
  *Y = t;
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

static inline u32 gcd(u32 a, u32 b) {
  u32 c = a % b;
  while (c) {
    a = b;
    b = c;
    c = a % b;
  }
  return b;
}

static inline u32 lcm(u32 a, u32 b) {
  return (a * b) / gcd(a, b);
}

#if (defined(__cplusplus))
}
#endif

#endif
