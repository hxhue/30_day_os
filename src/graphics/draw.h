#ifndef DRAW_H
#define DRAW_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <type.h>

typedef enum RGBColor {
  RGB_BLACK = 0,
  RGB_RED = 1,
  RGB_GREEN = 2,
  RGB_YELLOW = 3,
  RGB_CYAN = 4,
  RGB_PURPLE = 5,
  RGB_AQUA = 6,
  RGB_WHITE = 7,
  RGB_GRAY = 8,
  RGB_RED_DARK = 9,
  RGB_GREEN_DARK = 10,
  RGB_YELLOW_DARK = 11,
  RGB_CYAN_DARK = 12,
  RGB_PURPLE_DARK = 13,
  RGB_AQUA_DARK = 14,
  RGB_GRAY_DARK = 15,
  RGB_TRANSPARENT = 16,
} Color;

// extern u8 g_cursor_image[16][16];

typedef struct cursor_stat_t {
  i32 x, y;
} cursor_stat_t;

extern cursor_stat_t g_cursor_stat;

void init_palette(void);
void set_palette(int start, int end, const u8 *rgb);
void fill_rect(Color color, int x0, int y0, int x1, int y1);
void put_image(const u8 *rect, int width, int height, int x, int y);
void put_char(Color color, int x0, int y0, char ch);
/* Linewrap is not implemented. */
void put_string(Color color, int x0, int y0, const char *s);
void put_cursor(int x, int y);
void init_display();
void init_cursor();
void window_redraw_all();

#if (defined(__cplusplus))
	}
#endif

#endif
