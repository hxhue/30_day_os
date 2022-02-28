#ifndef LAYER_H
#define LAYER_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <support/type.h>
#include <event/mouse.h>

typedef struct layer_t layer_t;

struct layer_t {
  u8 focusable;         // Default: 1
  u16 width, height;    // Size
  u16 x, y;             // Position (top left)
  i16 rank;             // "rank" == 0 means invisible,
                        // because background's rank is 1
  u8 *buf;
  u64 last_active_time; // Last time the layer is active
};

// extern layer_ctl_t *g_lctl;

void init_layer_mgr();

int layer_pointer_cmp(void *lhs, void *rhs);

// Allocates an invisible layer. If "buf" == null, a new buffer will be
// allocated by the function.
layer_t *layer_new(int width, int height, int x, int y, u8 *buf);

// Set rank of the layer so it can be drawn. "rank" == 0 means invisible.
// Larger rank means being drawn later.
void layer_set_rank(layer_t *layer, i16 rank);

// Even more unsafe than layer_set_rank.
void layer_set_rank_no_bound(layer_t *layer, i16 rank);

// Set the max rank possible for given layer.
void layer_bring_to_front(layer_t *layer);

void layer_move_to(layer_t *layer, i32 x, i32 y);

// 0 on success, -1 on error
int layer_free(layer_t *layer);

// Redraw layers in given region.
void layers_redraw_all(int x0, int y0, int x1, int y1);

// Receive mouse event
void layers_receive_mouse_event(int x, int y, mouse_msg_t msg);

#if (defined(__cplusplus))
	}
#endif

#endif
