#include "support/xlibc.h"
#include <support/allocator.h>
#include <support/type.h>
#include <support/xlibc.h>
#include <string.h>

// "allocator_t" is for blocked data.
// Layout: [bitmap [allocatable blocks...]]
// Bitmap covers the whole buffer, including itself.
// Bitmap has paddings itself. Those bits will be set to 1.
// The area for bitmap is considered allocated and should be set to ones.
struct allocator_t {
  u8 *buffer;
  u32 bitmap_size;
  u32 buf_size;
  u32 ele_size;
};

void init_allocator(allocator_t *alloc, void *buf, unsigned bufsz,
                    unsigned elesz) {
  // Make "elesz" multiple of 8
  elesz = next_power_of2(elesz);
  if (elesz < 8)
    elesz = 8;
  // Make "bufsz" multiple of "elesz"
  u32 n = bufsz / elesz;
  bufsz = n * elesz;

  // Make "bmsz" multiple of "elesz", thus multiple of 8
  u32 bmsz = (n / 8 + elesz - 1) / elesz * elesz; 

  alloc->buffer = (u8 *)buf;
  memset(alloc->buffer, 0, bmsz);
  alloc->bitmap_size = bmsz;
  alloc->buf_size = bufsz;
  alloc->ele_size = elesz;

  // The first "times" bytes are used for bitmap itself.
  u32 times = bmsz / elesz;
  xassert(bmsz % elesz == 0);
  memset(alloc->buffer, -1, times / 8 * 8);
  alloc->buffer[times] |= ~((u8)-1 >> (times % 8));

  // The last few bits are just paddings. They exist because size of bitmap has
  // to be multiple of 8 bits.
  // TODO: 
}
