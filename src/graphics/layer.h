#ifndef LAYER_H
#define LAYER_H

#if (defined(__cplusplus))
	extern "C" {
#endif

#include <support/type.h>
#include <event/mouse.h>
#include <task/task.h>

typedef struct layer_t layer_t;

struct layer_t {
  // u8 focusable;         // Default: 1
  int width, height;    // Size
  int x, y;             // Position (top left)
  int rank;             // "rank" == 0 means invisible,
                        // because background's rank is 1
  u8 *buf;
  process_node_t *proc_node;
  u64 last_active_time; // Last time the layer is active
};

struct layer_msg_t {
  u8 focus;             // 1: Gain focus. 0: Lose focus.
  layer_t *layer;       // Which layer
};

typedef struct layer_msg_t layer_msg_t;

void init_layer_mgr();

int layer_pointer_cmp(void *lhs, void *rhs);

// Allocates an invisible layer. If "buf" == null, a new buffer will be
// allocated by the function.
layer_t *layer_new(process_node_t *pnode, int width, int height, int x, int y,
                   u8 *buffer);

// Set rank of the layer so it can be drawn. "rank" == 0 means invisible.
// Larger rank means being drawn later.
void layer_set_rank(layer_t *layer, int rank);

// Even unsafer than layer_set_rank.
void layer_set_rank_no_update(layer_t *layer, int rank);

// Set the max rank possible for given layer.
void layer_bring_to_front(layer_t *layer);

void layer_move_to(layer_t *layer, i32 x, i32 y);
void layer_move_by(layer_t *layer, i32 x, i32 y);

// 0 on success, -1 on error
int layer_free(layer_t *layer);

// Redraw layers in given region.
void layers_draw_all(int x0, int y0, int x1, int y1, u8 flags);

// Receive mouse event
void layers_receive_mouse_event(int x, int y, decoded_mouse_msg_t msg);

layer_t *layers_get_top();

#if (defined(__cplusplus))
	}
#endif

#endif
