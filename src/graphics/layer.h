#ifndef LAYER_H
#define LAYER_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <support/type.h>

#define MAX_LAYER_NUM    580
#define REDRAW_XY_FACTOR 8

typedef struct layer_info_t layer_info_t;
typedef struct layer_ctl_t layer_ctl_t;

struct layer_info_t {
  u8 inuse;
  u16 width, height; // Size
  u16 x, y;          // Position (top left)
  i16 rank;          // "rank" == 0 means invisible,
                     // because background's rank is 1
  u8 *buf;
};

extern layer_ctl_t *g_lctl;

void init_layer_mgr();

// Allocates an invisible layer. If "buf" == null, a new buffer will be
// allocated by the function.
layer_info_t *layer_new(int width, int height, int x, int y, u8 *buf);

// Set rank of the layer so it can be drawn. "rank" == 0 means invisible.
// Larger rank means being drawn later.
void layer_set_rank(layer_info_t *layer, i16 rank);

void layer_move_to(layer_info_t *layer, i32 x, i32 y);

// 0 on success, -1 on error
int layer_free(layer_info_t *layer);

// Redraw layers in given region.
void layer_redraw_all(int x0, int y0, int x1, int y1);

#if (defined(__cplusplus))
	}
#endif

#endif
