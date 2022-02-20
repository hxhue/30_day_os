#ifndef LAYER_H
#define LAYER_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <support/type.h>

#define MAX_LAYER_NUM 580

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
// extern layer_info_t *g_mouse_layer;

void init_layer_mgr();

// Allocates an invisible layer. If "buf" == null, a new buffer will be
// allocated by the function.
layer_info_t *new_layer(int width, int height, int x, int y, u8 *buf);

// Set rank of the layer so it can be drawn. "rank" == 0 means invisible.
// Larger rank means being drawn later.
void set_layer_rank(layer_info_t *layer, i16 rank);

void slide_layer(layer_info_t *layer, i32 x, i32 y);

// 0 on success, -1 on error
int delete_layer(layer_info_t *layer);

// Draw layer "base" and all layers above
void redraw_layers(layer_info_t *base, int x0, int y0, int x1, int y1);

#if (defined(__cplusplus))
	}
#endif

#endif
