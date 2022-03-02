#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <task/task.h>

typedef struct boot_info_t {
  u8 cyls, leds, vmode, reserve;
  u16 width, height;
  u32 vram_addr;
} boot_info_t;

extern boot_info_t g_boot_info;

typedef struct vbe_mode_info_t {
  u16 attributes; // deprecated, only bit 7 should be of interest to you, and it
                  // indicates the mode supports a linear frame buffer.
  u8 window_a;    // deprecated
  u8 window_b;    // deprecated
  u16 granularity; // deprecated; used while calculating bank numbers
  u16 window_size;
  u16 segment_a;
  u16 segment_b;
  u32 win_func_ptr; // deprecated; used to switch banks from protected mode
                    // without returning to real mode
  u16 pitch;        // number of bytes per horizontal line
  u16 width;        // width in pixels
  u16 height;       // height in pixels
  u8 w_char;        // unused...
  u8 y_char;        // ...
  u8 planes;
  u8 bpp;   // bits per pixel in this mode
  u8 banks; // deprecated; total number of banks in this mode
  u8 memory_model;
  u8 bank_size; // deprecated; size of a bank, almost always 64 KB but may be 16
                // KB...
  u8 image_pages;
  u8 reserved0;

  u8 red_mask;
  u8 red_position;
  u8 green_mask;
  u8 green_position;
  u8 blue_mask;
  u8 blue_position;
  u8 reserved_mask;
  u8 reserved_position;
  u8 direct_color_attributes;

  u32 framebuffer; // physical address of the linear frame buffer; write here to
                   // draw to the screen
  u32 off_screen_mem_off;
  u16 off_screen_mem_size; // size of memory in the framebuffer but not being
                           // displayed on the screen
  u8 reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

#if (defined(__cplusplus))
}
#endif

#endif
