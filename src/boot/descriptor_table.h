#ifndef DESCRIPTOR_TABLE_H
#define DESCRIPTOR_TABLE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <type.h>

typedef struct segment_descriptor_t {
  u16 limit_low, base_low;
  u8 base_mid, access;
  u8 limit_high : 4;
  u8 flag : 4;
  u8 base_high;
} segment_descriptor_t;

typedef struct gate_descriptor_t {
  u16 offset_low, selector;
  u8 dw_count, access;
  u16 offset_high;
} gate_descriptor_t;

void init_descriptor_tables();
void set_gdt_entry(segment_descriptor_t *entry, u32 limit, u32 base, u8 access,
                   u8 flag);
void set_idt_entry(gate_descriptor_t *entry, u32 offset, u32 selector,
                   u16 access);

#if (defined(__cplusplus))
}
#endif

#endif
