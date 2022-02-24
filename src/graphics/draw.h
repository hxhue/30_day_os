#ifndef DRAW_H
#define DRAW_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <support/type.h>
#include <event/event.h>

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

void init_display();

struct layer_info_t;
void draw_rect(struct layer_info_t *layer, Color color, int x0, int y0, int x1, int y1);
void draw_image(struct layer_info_t *layer, const u8 *rect, int width, int height, int x, int y);
void draw_char(struct layer_info_t *layer, Color color, int x0, int y0, char ch);
void draw_string(struct layer_info_t *layer, Color color, int x0, int y0, const char *s);

typedef struct region_t {
  int x0, y0, x1, y1;
} region_t;

extern event_queue_t g_redraw_event_queue;
void init_redraw_event_queue();
void emit_redraw_event(int x0, int y0, int x1, int y1);

#if (defined(__cplusplus))
	}
#endif

#endif
