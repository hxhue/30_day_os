#include "graphics/draw.h"
#include "boot/boot.h"
#include <boot/boot.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <support/type.h>
#include <support/debug.h>
#include <string.h>
#include <stdio.h>

#define USER_RANK_MAX 10000

struct layer_ctl_t {
  // Number of total layers and invisible layers
  u16 ntotal;

  // Needs to be allocated dynamically according to size of vram.
  // We calculate pixels here and copy them into "real" vram.
  u8 *vram;

  // Layers are sorted by "rank".
  // Smaller ranked layers are drawn first (background).
  // rank == 0 means layer is not visible.
  layer_info_t *layers     [MAX_LAYER_NUM];
  layer_info_t  layer_data [MAX_LAYER_NUM];
};

layer_ctl_t *g_lctl;

void full_redraw() {
  region_t region = {0, 0, g_boot_info.width, g_boot_info.height};
  emit_redraw_event(region);
}

// Requirements: x0 <= x1, y0 <= y1.
void partial_redraw(int x0, int y0, int x1, int y1) {
  region_t region = {x0, y0, x1, y1};
  emit_redraw_event(region);
}

static inline int layercmp(const layer_info_t *l, const layer_info_t *r) {
  return (l->rank != r->rank) ? (l->rank - r->rank) : l - r;
};

// Returns index of the layer ( -1 on failure).
// Binary search is useless because the moving operation's time complexity
// is O(n).
static int find_layer(layer_info_t *l) {
  layer_info_t **layers = g_lctl->layers;
  int i = 0, ntotal = g_lctl->ntotal;
  for (; i < ntotal; ++i) {
    if (layercmp(layers[i], l) == 0)
      return i;
  }
  return -1;
}

void init_layer_mgr() {
  g_lctl = (layer_ctl_t *)alloc_mem_4k(sizeof(layer_ctl_t));
  u32 vram_size = g_boot_info.height * g_boot_info.width;
  g_lctl->vram = (u8 *)alloc_mem_4k(vram_size);
  xassert(g_lctl);
  xassert(g_lctl->vram);
  memset(g_lctl->vram, 0, vram_size);
  g_lctl->ntotal = 0;
  int i;
  for (i = 0; i < MAX_LAYER_NUM; ++i) {
    g_lctl->layer_data[i].inuse = 0;
  }
}

// The layer pointer in g_lctl->layers[index] is a layer whose rank may have
// changed. All other layers have been ordered by their ranks. This function
// adjusts the index of the specified layer by swapping elements, and returns
// the new index of the layer.
static int adjust_layer_pos(int index) {
  layer_info_t **layers = g_lctl->layers;
  u32 ntotal = g_lctl->ntotal;

  xassert(sizeof(void *) == 4);

  while (index + 1 < ntotal && (layercmp(layers[index], layers[index + 1]) > 0)) {
    swapptr(&layers[index], &layers[index + 1]);
    ++index;
  }
  while (index > 0 && (layercmp(layers[index - 1], layers[index]) > 0)) {
    swapptr(&layers[index], &layers[index - 1]);
    --index;
  }
  return index;
}

layer_info_t *new_layer(int width, int height, int x, int y, u8 *buf) {
  xassert(width > 0 && height > 0);
  int i;
  for (i = 0; i < MAX_LAYER_NUM; ++i) {
    if (!g_lctl->layer_data[i].inuse) {
      if (!buf) {
        int sz = width * height;
        buf = (u8 *)alloc_mem(sz);
        int j;
        for (j = 0; j < sz; ++j)
          buf[j] = RGB_TRANSPARENT;
      }
      xassert(buf);
      layer_info_t *layer = &g_lctl->layer_data[i];
      *layer = (layer_info_t){
        .inuse = 1,
        .width = (u16)width,
        .height = (u16)height,
        .x = (u16)x,
        .y = (u16)y,
        .rank = 0,
        .buf = buf,
      };
      xassert(layer->buf);
      int index = g_lctl->ntotal++;
      g_lctl->layers[index] = layer;

      // A new layer is invisible.
      // Although we insert it into the array to keep the order,
      // we don't need to redraw the screen.
      adjust_layer_pos(index);

      return layer;
    }
  }
  return (layer_info_t *)0;
}

void set_layer_rank(layer_info_t *layer, i16 rank) {
  xassert(layer);
  
  // Find index. If rank is changed before this, we'll be unable
  // to find the location because order is violated.
  int index = find_layer(layer);

  layer->rank = clamp_i16(rank, 0, USER_RANK_MAX);

  adjust_layer_pos(index);
  partial_redraw(layer->x, layer->y, layer->x + layer->width,
                 layer->y + layer->height);
}

// Changes the position of the layer.
// Requirements: x >= 0, x < 65536, y >= 0, y < 65536
void move_layer_to(layer_info_t *layer, i32 x, i32 y) {
  int x0 = layer->x, y0 = layer->y;
  layer->x = x;
  layer->y = y;

  if (layer->rank <= 0) {
    return;
  }

  int w = layer->width, h = layer->height;
  int screen_size = g_boot_info.width * g_boot_info.height;
  if (w * h * 3 < screen_size) {
    partial_redraw(x0, y0, x0 + w, y0 + h);
    partial_redraw(x,  y,  x + w,  y + h);
  } else {
    full_redraw();
  }
}

int delete_layer(layer_info_t *layer) {
  // Find this layer and move all layers behind it forward.
  int index = find_layer(layer);
  if (index < 0) {
    return -1;
  }
  int ntotal = g_lctl->ntotal;
  layer_info_t **layers = g_lctl->layers;
  for (; index + 1 < ntotal; ++index) {
    // Move pointers
    layers[index] = layers[index + 1];
  }
  g_lctl->ntotal--;
  layer->inuse = 0;
  if (layer->rank > 0) {
    partial_redraw(layer->x, layer->y, layer->x + layer->width,
                   layer->y + layer->height);
  }
  return 0;
}

// (x0, y0) (Include) -> (x1, y1) (Exclude) is the area on the screen you want
// to redraw. The coordinates are based on the whole screen, not any of the
// possibly not-full-sreen layers.
void redraw_layers(int x0, int y0, int x1, int y1) {
  i32 winh = g_boot_info.height, winw = g_boot_info.width;
  u8 *vram = g_lctl->vram;
  x0 = clamp_i32(x0, 0, winw);
  y0 = clamp_i32(y0, 0, winh);
  x1 = clamp_i32(x1, 0, winw);
  y1 = clamp_i32(y1, 0, winh);

  // xprintf("Drawing (%d,%d,%d,%d)\n", x0, y0, x1, y1);

  int i;
  for (i = 0; i < g_lctl->ntotal; ++i) {
    layer_info_t *layer = g_lctl->layers[i];
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
