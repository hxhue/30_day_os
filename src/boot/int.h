#ifndef INT_H
#define INT_H

#if (defined(__cplusplus))
extern "C" {
#endif

typedef struct gate_descriptor_t {
  u16 offset_low, selector;
  u8 dw_count, access;
  u16 offset_high;
} gate_descriptor_t;

void init_interrupt();

#if (defined(__cplusplus))
}
#endif

#endif

