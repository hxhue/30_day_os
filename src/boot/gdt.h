#ifndef DESCRIPTOR_TABLE_H
#define DESCRIPTOR_TABLE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

typedef struct segment_descriptor_t {
  u16 limit_low, base_low;
  u8 base_mid, access;
  u8 limit_high : 4;
  u8 flag : 4;
  u8 base_high;
} segment_descriptor_t;

void init_gdt();

#if (defined(__cplusplus))
}
#endif

#endif
