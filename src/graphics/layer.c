#include "graphics/draw.h"
#include "boot/boot.h"
#include "memory/memory.h"
#include "support/tree.h"
#include "graphics/layer.h"
#include "support/debug.h"
#include "task/task.h"
#include <boot/boot.h>
#include <event/event.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stddef.h>
#include <support/type.h>
#include <support/debug.h>
#include <string.h>
#include <stdio.h>
#include <event/timer.h>
#include <memory/nalloc.h>

#define MAX_LAYER_NUM 2048
#define LAYER_RANK_MAX 127

typedef struct layer_ctl_t layer_ctl_t;

struct layer_ctl_t {
  u16 ntotal;

  // Needs to be allocated dynamically according to size of vram.
  // We calculate pixels here and copy them into "real" vram.
  u8 *vram;

  // Layers are sorted by "rank". Smaller ranked layers are drawn first
  // (background). rank == 0 means layer is not visible.
  tree_t layers;

  node_alloc_t tree_node_alloc;  // Stores pointers to layer_t
  node_alloc_t layer_info_alloc; 
};

static layer_ctl_t layerctl;

// Now we only store a pointer in tree node.
int layer_pointer_cmp(void *lhs, void *rhs) {
  layer_t *l = *(layer_t **)lhs;
  layer_t *r = *(layer_t **)rhs;

  if (l->rank != r->rank) {
    return l->rank - r->rank;
  }

  // Same rank
  if (l->last_active_time < r->last_active_time) return -1;
  if (l->last_active_time > r->last_active_time) return 1;

  // Same rank and last_active_time
  if ((size_t)l < (size_t)r) return -1;
  if ((size_t)l > (size_t)r) return 1;
  return 0;
};

static void *alloc_tree_node(size_t size) {
  xassert(size == tree_get_node_size(sizeof(void *)));
  // May be null.
  void *node = node_alloc_get(&layerctl.tree_node_alloc);
  return node;
}

static void free_tree_node(void *addr) {
  (void)node_alloc_reclaim(&layerctl.tree_node_alloc, addr);
}

void init_layer_mgr() {
  u32 vram_size = g_boot_info.height * g_boot_info.width;
  layerctl.vram = (u8 *)alloc_mem_4k(vram_size);
  xassert(layerctl.vram);
  memset(layerctl.vram, 0, vram_size);
  layerctl.ntotal = 0;
  u32 sz = MAX_LAYER_NUM * tree_get_node_size(sizeof(void *));
  void *mem = alloc_mem_4k(sz);
  xassert(mem);
  node_alloc_init(&layerctl.tree_node_alloc, mem, sz, tree_get_node_size(sizeof(void *)));
  tree_init(&layerctl.layers, sizeof(void *), alloc_tree_node, free_tree_node, layer_pointer_cmp);
  sz = MAX_LAYER_NUM * sizeof(layer_t);
  mem = alloc_mem_4k(sz);
  xassert(mem);
  node_alloc_init(&layerctl.layer_info_alloc, mem, sz, sizeof(layer_t));
}

layer_t *layer_new(process_node_t *pnode, int width, int height, int x, int y,
                   u8 *buffer) {
  xassert(width > 0 && height > 0);

  u8 *buf = (u8 *)0;
  layer_t *layer = (layer_t *)0;
  void *key = (void *)0;
  int success = 0;
  
  do {
    // Allocate buffer
    if (!buffer) {
      int sz = width * height;
      buf = (u8 *)alloc_mem_4k(sz);
      if (!buf) break;
      for (int i = 0; i < sz; ++i) {
        buf[i] = RGB_TRANSPARENT;
      }
      buffer = buf;
    }

    // Allocate layer node
    layer = node_alloc_get(&layerctl.layer_info_alloc);
    if (!layer) break;
    *layer = (layer_t) {
      .focusable = 1,
      .width = width,
      .height = height,
      .x = x,
      .y = y,
      .rank = 0, // Layers are invisible initially.
      .buf = buffer,
      .proc_node = pnode,
      .last_active_time = 0
    };

    // Insert pointer to the layer into the tree
    key = tree_insert(&layerctl.layers, &layer);
    if (key) success = 1;
  } while (0);

  if (!success) {
    if (buf)   reclaim_mem_4k(buf, width * height);
    if (layer) node_alloc_reclaim(&layerctl.layer_info_alloc, layer);
  }

  process_t *proc = get_proc_from_node(pnode);
  tree_insert(&proc->layers, &layer);

  return layer;
}

void layer_set_rank(layer_t *layer, i16 rank) {
  layer_set_rank_no_bound(layer, clamp_i16(rank, 0, LAYER_RANK_MAX));
}

void layer_set_rank_no_bound(layer_t *layer, i16 rank) {
  xassert(layer);
  void *key = tree_find(&layerctl.layers, &layer);

  if (!key) {
    xprintf("Cannot set the rank of an non-existing layer\n");
    return;
  }

  layer = *(layer_t **)key;
  layer->rank = rank;
  layer->last_active_time = g_counter.count;
  tree_update(&layerctl.layers, key);

  emit_redraw_event(layer->x, layer->y, layer->x + layer->width,
                    layer->y + layer->height);
}

void layer_bring_to_front(layer_t *layer) {
  layer_set_rank(layer, LAYER_RANK_MAX);
}

// Changes the position of the layer.
// Requirements: x >= 0, x < 65536, y >= 0, y < 65536
void layer_move_to(layer_t *layer, i32 x, i32 y) {
  int x0 = layer->x, y0 = layer->y;
  layer->x = x;
  layer->y = y;

  if (layer->rank <= 0) {
    return;
  }

  int w = layer->width, h = layer->height;
  int screen_size = g_boot_info.width * g_boot_info.height;
  if (w * h * 3 < screen_size) {
    emit_redraw_event(x0, y0, x0 + w, y0 + h);
    emit_redraw_event(x,  y,  x + w,  y + h);
  } else {
    emit_redraw_event(0, 0, g_boot_info.width, g_boot_info.height);
  }
}

// (x0, y0) (Include) -> (x1, y1) (Exclude) is the area on the screen you want
// to redraw. The coordinates are based on the whole screen, not any of the
// possibly not-full-sreen layers.
void layers_redraw_all(int x0, int y0, int x1, int y1) {
  i32 winh = g_boot_info.height, winw = g_boot_info.width;
  u8 *vram = layerctl.vram;
  x0 = clamp_i32(x0, 0, winw);
  y0 = clamp_i32(y0, 0, winh);
  x1 = clamp_i32(x1, 0, winw);
  y1 = clamp_i32(y1, 0, winh);

  // xprintf("Drawing 0X%p:(%d,%d,%d,%d)\n", vram, x0, y0, x1, y1);

  void *key;
  for (key = tree_smallest_key(&layerctl.layers); key;
       key = tree_next_key(&layerctl.layers, key)) {
    layer_t *layer = *(layer_t **)key;
    int maxy = min_i32(layer->height, y1 - layer->y);
    int maxx = min_i32(layer->width, x1 - layer->x);
    int miny = max_i32(0, -layer->y);
    int minx = max_i32(0, -layer->x);
    int x, y;
    for (y = miny; y < maxy; ++y) {
      u32 layer_offs = y * layer->width;
      u32 vram_offs = (y + layer->y) * winw;
      for (x = minx; x < maxx; ++x) {
        u8 color = layer->buf[layer_offs + x];
        if (color != RGB_TRANSPARENT)
          vram[vram_offs + x + layer->x] = color;
      }
    }
  }

  // Copy buffer to vga buffer
  memcpy((void *)g_boot_info.vram_addr, vram, winh * winw);
}

// TODO: Notify mouse when last_receiver_layer is destroyed. Or invalid memory
// access will happen.
static mouse_msg_t last_mouse_msg = {0};
static layer_t *last_mouse_receiver = NULL;

void layers_receive_mouse_event(int x, int y, mouse_msg_t msg) {
  // xprintf("Searching layers tree. Size: %d\n", tree_size(&layerctl.layers));
  layer_t *receiver = NULL;
  
  // When left button of mouse is clicked but never released, we should still
  // track the last layer.
  int lbutton_was_down = last_mouse_msg.buf[0] & 0x01;
  if (lbutton_was_down && last_mouse_receiver) {
    receiver = last_mouse_receiver;  
  }

  if (!receiver) {
    for (void *key = tree_largest_key(&layerctl.layers); key; 
        key = tree_prev_key(&layerctl.layers, key)) {
      layer_t *layer = *(layer_t **)key;
      // Skip mouse layer itself and whatever layers unclickable.
      if (layer->rank > LAYER_RANK_MAX) {
        continue;
      }
      if (x >= layer->x && x < (layer->x + layer->width) && 
          y >= layer->y && y < (layer->y + layer->height)) {
        receiver = layer;
        break;
      }
      // xprintf("Layer #%p is skipped by mouse\n", layer);
    }
  }

  if (receiver) {
    int btn = msg.buf[0] & 0x07;
    // Left button clicked
    if (btn & 0x01) {
      // Make the window focus
      if (receiver->focusable) {
        layer_bring_to_front(receiver);
        // TODO: notify the layer who loses focus
      }
    }
    process_t *proc = get_proc_from_node(receiver->proc_node);
    if (proc->irqmask & IRQBIT_MOUSE) {
      proc->irq |= IRQBIT_MOUSE;
      // TODO: send mouse event to process of the layer if its irq bit is set.
    }
  }

  last_mouse_msg = msg;
  last_mouse_receiver = receiver;
}

int layer_free(layer_t *layer) {
  // Save information before free
  u16 x = layer->x, y = layer->y;
  u16 w = layer->width, h = layer->height;
  i16 rank = layer->rank;
  if (tree_remove(&layerctl.layers, &layer) < 0) {
    return -1;
  }
  node_alloc_reclaim(&layerctl.layer_info_alloc, layer);
  layerctl.ntotal--;
  if (rank > 0) {
    emit_redraw_event(x, y, x + w, y + h);
  }
  if (layer == last_mouse_receiver) {
    last_mouse_receiver = NULL;
  }
  return 0;
}
