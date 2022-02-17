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

#if (defined(__cplusplus))
}
#endif

#endif
