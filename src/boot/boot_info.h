#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

typedef struct boot_info_t {
  u8 cyls, leds, vmode, reserve;
  u16 width, height;
  u32 vram_addr;
} boot_info_t;

extern boot_info_t g_boot_info;

#if (defined(__cplusplus))
}
#endif

#endif
