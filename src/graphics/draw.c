#include <graphics/draw.h>
#include <boot/boot_info.h>
#include <inst.h>
#include <type.h>
#include <event/event.h>

static void draw_background() {
  int x = g_boot_info.width;
  int y = g_boot_info.height;

  fill_rect(RGB_AQUA_DARK, 0,      0,          x,  y - 28);
  fill_rect(RGB_GRAY,      0,      y - 28,     x,  y - 27);
  fill_rect(RGB_WHITE,     0,      y - 27,     x,  y - 26);
  fill_rect(RGB_GRAY,      0,      y - 26,     x,  y);
  fill_rect(RGB_WHITE,     3,      y - 24,    60,  y - 23);
  fill_rect(RGB_WHITE,     2,      y - 24,     3,  y - 3);
  fill_rect(RGB_GRAY_DARK, 3,      y - 4,     60,  y - 3);
  fill_rect(RGB_GRAY_DARK, 59,     y - 23,    60,  y - 4);
  fill_rect(RGB_BLACK,     2,      y - 3,     60,  y - 2);
  fill_rect(RGB_BLACK,     60,     y - 24,    61,  y - 2);
  fill_rect(RGB_GRAY_DARK, x - 47, y - 24, x - 3,  y - 23);
  fill_rect(RGB_GRAY_DARK, x - 47, y - 23, x - 46, y - 3);
  fill_rect(RGB_WHITE,     x - 47, y - 3,  x - 3,  y - 2);
  fill_rect(RGB_WHITE,     x - 3,  y - 24, x - 2,  y - 2);

  put_char(RGB_WHITE, 0, 0, 'A');
  put_string(RGB_AQUA, 24, 0, "Day 6: Hello, world!");
}

void init_display() {
  init_palette();
  init_cursor();
  raise_event((event_t){.type = EVENT_REDRAW, .data = 0});
}

void put_char(Color color, int x0, int y0, char ch) {
  extern char hankaku[4096];
  u8 *font16x8 = &hankaku[ch * 16];
  u8 *vram = (u8 *)g_boot_info.vram_addr;
  int row;
  for (row = 0; row < 16; ++row) {
    u8 *row_addr = vram + g_boot_info.width * (row + y0) + x0;
    u8 byte = font16x8[row];
    if (byte & 0x80)
      row_addr[0] = (u8)color;
    if (byte & 0x40)
      row_addr[1] = (u8)color;
    if (byte & 0x20)
      row_addr[2] = (u8)color;
    if (byte & 0x10)
      row_addr[3] = (u8)color;
    if (byte & 0x08)
      row_addr[4] = (u8)color;
    if (byte & 0x04)
      row_addr[5] = (u8)color;
    if (byte & 0x02)
      row_addr[6] = (u8)color;
    if (byte & 0x01)
      row_addr[7] = (u8)color;
  }
}

void put_string(Color color, int x0, int y0, const char *s) {
  for (; *s; (void)++s, (void)(x0 += 8)) {
    put_char(color, x0, y0, *s);
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
    "..*OOOOO*.......", //
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

cursor_stat_t g_cursor_stat;

void init_cursor() {
  // Image
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
  // Position
  g_cursor_stat.x = g_boot_info.width / 2;
  g_cursor_stat.y = g_boot_info.height / 2;
}

void put_cursor(int x, int y) {
  int i, j;
  u8 *vram = (u8 *)g_boot_info.vram_addr;
  for (j = 0; j < CURSOR_HEIGHT; ++j) {
    for (i = 0; i < CURSOR_WIDTH; ++i) {
      u8 color = cursor_image[j][i];
      if (color != RGB_TRANSPARENT) {
        int x1 = i + x, y1 = j + y;
        if (x1 < g_boot_info.width && y1 < g_boot_info.height) {
          vram[y1 * g_boot_info.width + x1] = color;
        }
      }
    }
  }
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

/* Registers 16 predefined rgb colors by writing the first color index, and then
   writing rgb consecutively. Ref: https://wiki.osdev.org/VGA_Hardware */
void set_palette(int start, int end, const u8 *rgb) {
  int i, eflags;
  eflags = asm_load_eflags(); /* Bit 9 contains interrupt permission. */
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
  return;
}

/* Fills the rectangle with specified color index. The rectange area is defined
   by [x0, x1) and [y0, y1). */
void fill_rect(Color color, int x0, int y0, int x1, int y1) {
  u8 *vram = (u8 *)g_boot_info.vram_addr;
  int x, y;
  for (x = x0; x < x1; ++x)
    for (y = y0; y < y1; ++y)
      vram[g_boot_info.width * y + x] = (char)color;
}

/* Copies a rectangle with its top left relocated to (x, y). */
void put_image(const u8 *rect, int width, int height, int x, int y) {
  u8 *vram = (u8 *)g_boot_info.vram_addr;
  int i, j;
  for (j = 0; j < height; ++j) {
    int vram_offset = g_boot_info.width * (j + y);
    int rect_offset = j * width;
    for (i = 0; i < width; ++i) {
      vram[vram_offset + i + x] = rect[rect_offset + i];
    }
  }
}

// 2022年2月17日: background + mouse
void window_redraw_all() {
  draw_background();
  put_cursor(g_cursor_stat.x, g_cursor_stat.y);
}