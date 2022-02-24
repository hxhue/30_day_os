#ifndef MEMORY_H
#define MEMORY_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

void init_mem_mgr();

u32  get_max_mem_addr();
void *alloc_mem(unsigned long size);
void *alloc_mem_4k(unsigned long size);
int  reclaim_mem(void *addr, unsigned long size); // 0 for success, -1 for failure
int  reclaim_mem_4k(void *addr, unsigned long size);
void reclaim_mem_no_return_value(void *addr, unsigned long size);
u32  get_avail_mem();

#if (defined(__cplusplus))
}
#endif

#endif

