#ifndef MEMORY_H
#define MEMORY_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

void init_mem_mgr();

u32  get_max_mem_addr();
void *alloc_mem(u32 size);
void *alloc_mem_4k(u32 size);
int  reclaim_mem(u32 addr, u32 size); // 0 for success, -1 for failure
int  reclaim_mem_4k(u32 addr, u32 size);
u32  get_avail_mem();

#if (defined(__cplusplus))
}
#endif

#endif

