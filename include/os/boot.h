#ifndef BOOT_H
#define BOOT_H

struct boot_info_t {
  char cyls, leds, vmode, reserve;
  short width, height;
  char *vram;
};

extern struct boot_info_t g_boot_info;

#endif
