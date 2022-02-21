#include <boot/boot.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stdio.h>
#include <string.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/xlibc.h>

// TODO: Mouse layer is still user layer
#define MOUSE_LAYER_RANK 20000

layer_info_t *g_mouse_layer;
static layer_info_t *g_background_layer;

void init_palette();
void init_cursor();

void init_background() {
  int w = g_boot_info.width;
  int h = g_boot_info.height;

  layer_info_t *layer = new_layer(w, h, 0, 0, (u8 *)0);
  xassert(layer);

  fill_rect(layer, RGB_AQUA_DARK, 0,      0,          w,  h - 28);
  fill_rect(layer, RGB_GRAY,      0,      h - 28,     w,  h - 27);
  fill_rect(layer, RGB_WHITE,     0,      h - 27,     w,  h - 26);
  fill_rect(layer, RGB_GRAY,      0,      h - 26,     w,  h);
  fill_rect(layer, RGB_WHITE,     3,      h - 24,    60,  h - 23);
  fill_rect(layer, RGB_WHITE,     2,      h - 24,     3,  h - 3);
  fill_rect(layer, RGB_GRAY_DARK, 3,      h - 4,     60,  h - 3);
  fill_rect(layer, RGB_GRAY_DARK, 59,     h - 23,    60,  h - 4);
  fill_rect(layer, RGB_BLACK,     2,      h - 3,     60,  h - 2);
  fill_rect(layer, RGB_BLACK,     60,     h - 24,    61,  h - 2);
  fill_rect(layer, RGB_GRAY_DARK, w - 47, h - 24, w - 3,  h - 23);
  fill_rect(layer, RGB_GRAY_DARK, w - 47, h - 23, w - 46, h - 3);
  fill_rect(layer, RGB_WHITE,     w - 47, h - 3,  w - 3,  h - 2);
  fill_rect(layer, RGB_WHITE,     w - 3,  h - 24, w - 2,  h - 2);

  put_char(layer, RGB_WHITE, 0, 0, 'A');
  put_string(layer, RGB_AQUA, 24, 0, "Day 6: Hello, world!");

  u32 max_addr = get_max_mem_addr();
  u32 free_mem = get_avail_mem();
  char buf[64];
  sprintf(buf, "memory: %d MB, free: %d KB", max_addr / (1024 * 1024),
          free_mem / 1024);
  put_string(layer, RGB_WHITE, 0, 80, buf);
  sprintf(buf, "rand: %d", xrand());
  put_string(layer, RGB_WHITE, 0, 96, buf);

  set_layer_rank(layer, 1);

  g_background_layer = layer;
}

void init_display() {
  init_palette();
  init_layer_mgr();
  init_background(); // Background layer
  init_cursor();     // Cursor layer
}

#define HANKAKU_CHAR_WIDTH  8
#define HANKAKU_CHAR_HEIGHT 16

// Requirements:
// - layer and color are valid
// - x0 >= 0, y0 >= 0 (For performance)
void put_char(layer_info_t *layer, Color color, int x0, int y0, char ch) {
  xassert(x0 >= 0 && y0 >= 0);
  extern char hankaku[4096];
  u8 *font16x8 = (u8 *)&hankaku[ch * HANKAKU_CHAR_HEIGHT];
  u8 *vram = layer->buf;
  int maxh = min_i32(layer->height - y0, HANKAKU_CHAR_HEIGHT);
  int maxw = min_i32(layer->width - x0, HANKAKU_CHAR_WIDTH);
  int h;
  for (h = 0; h < maxh; ++h) {
    u8 *row = vram + layer->width * (h + y0) + x0;
    u8 byte = font16x8[h];
    // No break
    switch (maxw - 1) {
      case  7: if (byte & 0x01) row[7] = (u8)color;
      case  6: if (byte & 0x02) row[6] = (u8)color;
      case  5: if (byte & 0x04) row[5] = (u8)color;
      case  4: if (byte & 0x08) row[4] = (u8)color;
      case  3: if (byte & 0x10) row[3] = (u8)color;
      case  2: if (byte & 0x20) row[2] = (u8)color;
      case  1: if (byte & 0x40) row[1] = (u8)color;
      case  0: if (byte & 0x80) row[0] = (u8)color;
      default: break;
    }
  }
}

// Requirements:
// - layer and color are valid
// - x0 >= 0, y0 >= 0 (For performance)
// - s is not empty and accessible
void put_string(layer_info_t *layer, Color color, int x0, int y0, const char *s) {
  xassert(x0 >= 0 && y0 >= 0);
  for (; *s; (void)++s, (void)(x0 += 8)) {
    put_char(layer, color, x0, y0, *s);
  }
}

#define CURSOR_WIDTH  16
#define CURSOR_HEIGHT 16

/* Comments are used to prevent tools from formating the code. */
static u8 cursor_image[CURSOR_HEIGHT][CURSOR_WIDTH] = {
    "***.............", //
    "*OO**...........", //
    "*OOOO***........", //
    ".*OOOOOO**......", //
    ".*OOOOOOOO*.....", //
    ".*OOOOOOO*......", //
    ".**OOOOO*.......", //
    "..*OOOOOO*......", //
    "..*OO*OOOO*.....", //
    "..*OO*.*OOO*....", //
    "..*O*...*OOO*...", //
    ".........*OOO*..", //
    "..........*OOO*.", //
    "...........*OOO*", //
    "............*OO*", //
    ".............***"  //
};

void init_cursor() {
  // Create cursor image
  int x, y;
  for (y = 0; y < CURSOR_HEIGHT; ++y) {
    for (x = 0; x < CURSOR_WIDTH; ++x) {
      switch (cursor_image[y][x]) {
      case '*':
        cursor_image[y][x] = RGB_BLACK;
        break;
      case 'O':
        cursor_image[y][x] = RGB_WHITE;
        break;
      case '.':
        cursor_image[y][x] = RGB_TRANSPARENT;
        break;
      default:
        break;
      }
    }
  }

  g_mouse_layer = new_layer(CURSOR_WIDTH, CURSOR_HEIGHT, g_boot_info.width / 2,
                            g_boot_info.height / 2, &cursor_image[0][0]);
  xassert(g_mouse_layer);
  set_layer_rank(g_mouse_layer, MOUSE_LAYER_RANK);
}

/* Registers 16 predefined rgb colors by writing the first color index, and then
   writing rgb consecutively. Ref: https://wiki.osdev.org/VGA_Hardware */
static inline void set_palette(int start, int end, const u8 *rgb) {
  int i;
  u32 eflags = asm_load_eflags(); /* Bit 9 contains interrupt permission. */
  asm_cli();                  /* Clear interrupt permission. */
  asm_out8(0x03c8, start);
  for (i = start; i <= end; i++) {
    asm_out8(0x03c9, rgb[0] / 4);
    asm_out8(0x03c9, rgb[1] / 4);
    asm_out8(0x03c9, rgb[2] / 4);
    rgb += 3;
  }
  /* Restore eflags and thus the interrupt permission. */
  asm_store_eflags(eflags);
}

void init_palette() {
  static const u8 table_rgb[16 * 3] = {
      0x00, 0x00, 0x00, /*  0:黒 */
      0xff, 0x00, 0x00, /*  1:明るい赤 */
      0x00, 0xff, 0x00, /*  2:明るい緑 */
      0xff, 0xff, 0x00, /*  3:明るい黄色 */
      0x00, 0x00, 0xff, /*  4:明るい青 */
      0xff, 0x00, 0xff, /*  5:明るい紫 */
      0x00, 0xff, 0xff, /*  6:明るい水色 */
      0xff, 0xff, 0xff, /*  7:白 */
      0xc6, 0xc6, 0xc6, /*  8:明るい灰色 */
      0x84, 0x00, 0x00, /*  9:暗い赤 */
      0x00, 0x84, 0x00, /* 10:暗い緑 */
      0x84, 0x84, 0x00, /* 11:暗い黄色 */
      0x00, 0x00, 0x84, /* 12:暗い青 */
      0x84, 0x00, 0x84, /* 13:暗い紫 */
      0x00, 0x84, 0x84, /* 14:暗い水色 */
      0x84, 0x84, 0x84  /* 15:暗い灰色 */
  };
  set_palette(0, 15, table_rgb);
}

/* Fills the rectangle with specified color index. The rectange area is defined
   by (x0, y0) (Include) -> (x1, y1) (Exclude). */
void fill_rect(layer_info_t *layer, Color color, int x0, int y0, int x1, int y1) {
  u8 *vram = layer->buf;
  x0 = max_i32(x0, 0);
  y0 = max_i32(y0, 0);
  x1 = min_i32(x1, layer->width);
  y1 = min_i32(y1, layer->height);
  int x, y;
  for (y = y0; y < y1; ++y) {
    int offset = layer->width * y;
    for (x = x0; x < x1; ++x) {
      vram[offset + x] = (char)color;
    }
  }
}

// Copies a rectangle with its top left relocated to (x, y).
// Requirements:
// - layer is valid
// - rect is accessible, and holds at least width * height bytes
// - width >= 0, height >= 0
void put_image(layer_info_t *layer, const u8 *rect, int width, int height, int x, int y) {
  u8 *vram = layer->buf;
  int minj = max_i32(0, -y);
  int mini = max_i32(0, -x);
  int maxj = min_i32(height, layer->height - y);
  int maxi = min_i32(width, layer->width - x);
  int i, j;
  for (j = minj; j < maxj; ++j) {
    int vram_offset = layer->width * (j + y);
    int rect_offset = j * width;
    for (i = mini; i < maxi; ++i) {
      vram[vram_offset + i + x] = rect[rect_offset + i];
    }
  }
}

void handle_event_redraw(int data) {
  redraw_layers(0, 0, 0, g_boot_info.width, g_boot_info.height);
}
