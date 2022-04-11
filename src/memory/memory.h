#ifndef MEMORY_H
#define MEMORY_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <stddef.h>

void init_mem_mgr();
u32  get_max_mem_addr();
u32  get_avail_mem();

// Return values of alloc_mem* family: 0 on failure, other values on success
void *alloc_mem(unsigned long size);
void *alloc(size_t size);
void *alloc_mem_4k(unsigned long size);

// Return values of reclaim_mem* family: 0 on success, -1 on failure
int  reclaim_mem(void *addr, unsigned long size); 
void reclaim(void *addr);
int  reclaim_mem_4k(void *addr, unsigned long size);
void reclaim_mem_no_return_value(void *addr, unsigned long size);

#if (defined(__cplusplus))
}
#endif

#endif

