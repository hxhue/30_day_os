#include <memory/memory.h>
#include <stddef.h>
#include <support/asm.h>
#include <support/debug.h>
#include <support/type.h>

#define EFLAGS_AC_BIT     0x00040000
#define CR0_CACHE_DISABLE 0x60000000

#define MEM_SET_ENTRIES 4096
#define MEM_SET_ADDR 0x003c0000

typedef struct memory_entry_t {
  void *addr;
  u32 size;
} memory_entry_t;

typedef struct memory_pool_t {
  memory_entry_t entries[MEM_SET_ENTRIES];
  u32 frees, maxfrees, lostsize, losts;
} memory_pool_t;

static memory_pool_t *g_mem_pool;

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

void *alloc_mem(unsigned long size) {
  unsigned int i;
  void *a;
  memory_pool_t *mem = g_mem_pool;
  for (i = 0; i < mem->frees; i++) {
    if (mem->entries[i].size >= size) {
      a = mem->entries[i].addr;
      mem->entries[i].addr = (void *)((u32)mem->entries[i].addr + size);
      mem->entries[i].size -= size;
      if (mem->entries[i].size == 0) {
        mem->frees--;
        for (; i < mem->frees; i++) {
          mem->entries[i] = mem->entries[i + 1];
        }
      }
      return a;
    }
  }
  return (void *)0;
}

void *alloc(size_t size) {
  size_t *p = (size_t *)alloc_mem(size + sizeof(size_t));
  if (p) {
    *p = size;
    return p + 1;
  }
  return (void *)0;
}

void reclaim(void *addr) {
  if (addr != 0) {
    size_t *real_addr = (size_t *)addr - 1;
    reclaim_mem(real_addr, *real_addr);
  }
}

int reclaim_mem(void *addr, unsigned long size) {
  if (addr == 0) {
    return 0;
  }
  memory_pool_t *mem = g_mem_pool;
  u32 i, j;
  for (i = 0; i < mem->frees; i++) {
    if (mem->entries[i].addr > addr) {
      break;
    }
  }
  /* free[i - 1].addr < addr < free[i].addr */
  if (i > 0) {
    if (mem->entries[i - 1].addr + mem->entries[i - 1].size == addr) {
      mem->entries[i - 1].size += size;
      if (i < mem->frees) {
        if (addr + size == mem->entries[i].addr) {
          mem->entries[i - 1].size += mem->entries[i].size;
          mem->frees--;
          for (; i < mem->frees; i++) {
            mem->entries[i] = mem->entries[i + 1];
          }
        }
      }
      return 0; /* 成功終了 */
    }
  }
  if (i < mem->frees) {
    if (addr + size == mem->entries[i].addr) {
      mem->entries[i].addr = addr;
      mem->entries[i].size += size;
      return 0; /* 成功終了 */
    }
  }
  if (mem->frees < MEM_SET_ENTRIES) {
    for (j = mem->frees; j > i; j--) {
      mem->entries[j] = mem->entries[j - 1];
    }
    mem->frees++;
    if (mem->maxfrees < mem->frees) {
      mem->maxfrees = mem->frees;
    }
    mem->entries[i].addr = addr;
    mem->entries[i].size = size;
    return 0; /* 成功終了 */
  }
  mem->losts++;
  mem->lostsize += size;
  return -1; /* 失敗終了 */
}

u32 get_avail_mem() {
  memory_pool_t *mem = g_mem_pool;
  unsigned int i, t = 0;
  for (i = 0; i < mem->frees; i++) {
    t += mem->entries[i].size;
  }
  return t;
}

void init_mem_mgr() {
  g_mem_pool = (memory_pool_t *)MEM_SET_ADDR;
  reclaim_mem((void *)0x00001000, 0x0009e000);
  u32 max_addr = get_max_mem_addr();
  reclaim_mem((void *)0x00400000, max_addr - 0x00400000);
}

void *alloc_mem_4k(unsigned long size) {
  size = (size + 0xfff) & 0xfffff000;
  return alloc_mem(size);
}

int reclaim_mem_4k(void *addr, unsigned long size) {
  size = (size + 0xfff) & 0xfffff000;
  return reclaim_mem(addr, size);
}

void reclaim_mem_no_return_value(void *addr, unsigned long size) {
  (void)reclaim_mem(addr, size);
}
