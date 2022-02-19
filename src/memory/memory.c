#include <memory/memory.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/xlibc.h>

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

#define NUM_MEM_SET_ENTRIES 4096

typedef struct memory_entry_t {
  u32 addr;
  u32 size;  // 0 means this is a dummy entry
  u16 next;  // Index of next entry (in "entries")
  u8  inuse; // 1: in use; 0: free
} memory_entry_t;

#if (NUM_MEM_SET_ENTRIES > 65535 || (NUM_MEM_SET_ENTRIES % 8) != 0)
#error
#endif

struct memory_set_t {
  memory_entry_t entries[NUM_MEM_SET_ENTRIES];
  u32 total; // Total memory size (bytes)
  u32 inuse; // Memory usage (bytes)
};

u32 get_max_mem_addr_impl(u32 start, u32 end) {
  u32 i, old;
  const u32 pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
  volatile u32 *p;
  // 0x1000: a larger step makes the check faster
  for (i = start; i < end; i += 0x1000) {
    // Check the last 4 bytes in 4 KB
    p = (u32 *)(i + 0xffc);
    // Save value
    old = *p;
    *p = pat0;
    *p ^= 0xffffffff;
    // end = 0 makes the loop exit
    if (*p != pat1)
      end = 0;
    *p ^= 0xffffffff;
    if (*p != pat0)
      end = 0;
    *p = old;
  }
  return i;
}

// Get max available memory address.
// Start: 0x00400000
// End:   0xbfffffff
// The test is done by reading and writing the memory util the process fails
// or the end is reached. Although the address space is large, much of it
// is protected, so we can only use a small part of it.
u32 get_max_mem_addr() {
  static u32 cached_addr = 0;
  if (cached_addr)
    return cached_addr;

  int flag486 = 0;

  // Determine architecture:
  // Setting AC bit for arch 386 has no effect, so we can use this feature to
  // differentiate 386 and 486.
  u32 eflag = asm_load_eflags();
  eflag |= EFLAGS_AC_BIT;
  asm_store_eflags(eflag);
  eflag = asm_load_eflags();
  if (eflag & EFLAGS_AC_BIT)
    flag486 = 1;
  eflag &= ~EFLAGS_AC_BIT;
  asm_store_eflags(eflag);

  // Disable cache for arch 486. So we can really test the memory.
  if (flag486) {
    u32 cr0 = asm_load_cr0();
    cr0 |= CR0_CACHE_DISABLE;
    asm_store_cr0(cr0);
  }

  u32 mem = get_max_mem_addr_impl(0x00400000, 0xbfffffff);
  cached_addr = mem;

  if (flag486) {
    u32 cr0 = asm_load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE;
    asm_store_cr0(cr0);
  }

  return mem;
}

void init_mem(memory_set_t *mem, u32 addr, u32 total_size) {
  xassert(addr > 0 && total_size > 0 && mem);
  mem->total = total_size;
  mem->inuse = 0;
  // TODO:
}

u32 alloc_mem(memory_set_t *mem, u32 size) {
  return 0;
}

void free_mem(memory_set_t *mem, u32 addr) {}
