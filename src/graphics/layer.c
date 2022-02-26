#include "graphics/draw.h"
#include "boot/boot.h"
#include "memory/node_allocator.h"
#include "memory/memory.h"
#include "support/tree.h"
#include "graphics/layer.h"
#include "support/debug.h"
#include <boot/boot.h>
#include <event/event.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stddef.h>
#include <support/type.h>
#include <support/debug.h>
#include <string.h>
#include <stdio.h>
#include <memory/node_allocator.h>

#define MAX_LAYER_NUM 2048
#define USER_RANK_MAX 10000

typedef struct layer_ctl_t layer_ctl_t;

struct layer_ctl_t {
  i16 next_id;
  // Number of total layers and invisible layers
  u16 ntotal;

  // Needs to be allocated dynamically according to size of vram.
  // We calculate pixels here and copy them into "real" vram.
  u8 *vram;

  // Layers are sorted by "rank". Smaller ranked layers are drawn first
  // (background). rank == 0 means layer is not visible.
  tree_t layers;

  node_allocator_t tree_node_alloc;  // Stores pointers to layer_info_t
  node_allocator_t layer_info_alloc; 
};

static layer_ctl_t layerctl;

// Now we only store a pointer in tree node.
static int layer_pointer_cmp(void *lhs, void *rhs) {
  layer_info_t *l = *(layer_info_t **)lhs;
  layer_info_t *r = *(layer_info_t **)rhs;
  return (l->rank != r->rank) ? (l->rank - r->rank) : l->id - r->id;
};

static void *alloc_tree_node(size_t size) {
  xassert(size == tree_get_node_size(sizeof(void *)));
  xprintf("[alloc_tree_node]\n");
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
  sz = MAX_LAYER_NUM * sizeof(layer_info_t);
  mem = alloc_mem_4k(sz);
  xassert(mem);
  node_alloc_init(&layerctl.layer_info_alloc, mem, sz, sizeof(layer_info_t));
}

layer_info_t *layer_new(int width, int height, int x, int y, u8 *buf) {
  xassert(width > 0 && height > 0);
  // Allocate buffer
  if (!buf) {
    int sz = width * height;
    buf = (u8 *)alloc_mem(sz);
    if (!buf) {
      return (layer_info_t *)0;
    }
    int i;
    for (i = 0; i < sz; ++i) {
      buf[i] = RGB_TRANSPARENT;
    }
  }

  // Allocate layer node
  layer_info_t *layer = node_alloc_get(&layerctl.layer_info_alloc);
  if (!layer) {
    reclaim_mem(buf, width * height);
    return (layer_info_t *)0;
  }
  *layer = (layer_info_t) {
    .width = width,
    .height = height,
    .x = x,
    .y = y,
    .rank = 0, // Layers are invisible initially.
    .buf = buf,
  };

  // Insert pointer to the layer into the tree
  void *key = tree_insert(&layerctl.layers, &layer);
  if (!key) { // Out of memory
    reclaim_mem(buf, width * height);
    return (layer_info_t *)0;
  }

  return layer;
}

void layer_set_rank(layer_info_t *layer, i16 rank) {
  xassert(layer);
  void *key = tree_find(&layerctl.layers, &layer);

  if (!key) {
    xprintf("Cannot set the rank of an non-existing layer\n");
    return;
  }

  layer = *(layer_info_t **)key;
  layer->rank = clamp_i16(rank, 0, USER_RANK_MAX);
  tree_update(&layerctl.layers, key);

  emit_redraw_event(layer->x, layer->y, layer->x + layer->width,
                    layer->y + layer->height);
}

// Changes the position of the layer.
// Requirements: x >= 0, x < 65536, y >= 0, y < 65536
void layer_move_to(layer_info_t *layer, i32 x, i32 y) {
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

int layer_free(layer_info_t *layer) {
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
  return 0;
}

// (x0, y0) (Include) -> (x1, y1) (Exclude) is the area on the screen you want
// to redraw. The coordinates are based on the whole screen, not any of the
// possibly not-full-sreen layers.
void layer_redraw_all(int x0, int y0, int x1, int y1) {
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
    layer_info_t *layer = *(layer_info_t **)key;
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
