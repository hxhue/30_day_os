#ifndef MEMORY_H
#define MEMORY_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

typedef struct memory_set_t memory_set_t;

u32  get_max_mem_addr();
// No need to call this util "malloc" is to be used.
void init_mem(memory_set_t *mem, u32 addr, u32 total_size);
u32  alloc_mem(memory_set_t *mem, u32 size);
void free_mem(memory_set_t *mem, u32 addr);
u32  get_avail_mem(const memory_set_t *mem);

#if (defined(__cplusplus))
}
#endif

#endif

