// Node allocator is used for managing a continous piece of memory and
// allocating elements one by one. Every element should be the same size, which
// is why this allocator is suitable for node (e.g. list node or tree node)
// allocation. Node allocator will not release the memory itself. It needs to be
// released by the memory provider.

#include <stddef.h>
#include <string.h>
#include "node_allocator.h"
#include <support/debug.h>

void node_alloc_init(node_allocator_t *alloc, void *memory, size_t memory_size,
                     size_t element_size) {
  alloc->element_size = element_size;
  alloc->real_size = memory_size;
  alloc->bmsz = memory_size / (1 + 8 * element_size);
  alloc->mem = (unsigned char *)memory;
  alloc->fast_count = 0;
  memset(alloc->mem, -1, alloc->bmsz);
}

void *node_alloc_get(node_allocator_t *alloc) {
  int found = 0;
  size_t bit, i;
  
  xprintf("node_alloc_get(): fast_count=%d, bmsz=%d\n", (int)alloc->fast_count, (int)alloc->bmsz);
  
  if (alloc->fast_count < alloc->bmsz * 8) {
    bit = alloc->fast_count++;
    alloc->mem[bit / 8] &= ~(0x80U >> (bit % 8));
    found = 1;
  }

  // TODO: there is a bug that when fast_count is not used, the same memory will be allocated twice!

  for (i = 0; !found && i < alloc->bmsz; ++i) {
    if (alloc->mem[i]) {
      unsigned char byte = alloc->mem[i];
      unsigned char x = byte & (-byte);
      alloc->mem[i] &= (~x);
      // For a byte, MSB is bit 0, while LSB is bit 7
      bit = 0; 
      if (byte & 0x0f) bit += 4;
      if (byte & 0x33) bit += 2;
      if (byte & 0x55) bit += 1;
      bit += i * 8;
      found = 1;
      break;
    }
  }
  if (found) {
    xprintf("node_alloc_get(): bit: %d\n", (int)bit);
    return alloc->mem + alloc->bmsz + bit * alloc->element_size;
  }
  return (void *)0; // Out of memory
}

int node_alloc_reclaim(node_allocator_t *alloc, void *addr) {
  size_t size = 1 + 8 * alloc->element_size;
  if (addr < (void *)(alloc->mem + alloc->bmsz) ||
      addr >= (void *)(alloc->mem + size)) {
    return -1;
  }
  size_t offset = (size_t)((unsigned char *)addr - alloc->mem - alloc->bmsz);
  if (offset % alloc->element_size != 0) {
    return -1;
  }
  size_t i = offset / alloc->element_size;
  unsigned char byte = 0x80U >> (i % 8);
  if (alloc->mem[i/8] & byte) {
    return -1;
  }
  alloc->mem[i/8] |= byte;
  return 0;
}
