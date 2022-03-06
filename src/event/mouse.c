#include "event/mouse.h"
#include <boot/boot.h>
#include <event/event.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <support/type.h>
#include <support/debug.h>
#include <support/queue.h>
#include <event/keyboard.h>
#include <boot/def.h>

void init_mouse() {
  wait_kbdc_ready();
  // Send command: transfer data to mouse
  asm_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_kbdc_ready();
  // Send data: tell mouse to start working
  asm_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}

// For a mouse initialized in this system, a packet consists 3 bytes.
// (For other systems, mouse may have an optional 4 bytes)
//
// First byte:
// [Y overflow]	[X overflow]	[Y sign bit]	[X sign bit]	
// [Always 1]	  [Middle Btn]	[Right Btn]	  [Left Btn]
//
// Because of the existence of an extra sign bit, movement is a 9-bit signed
// number.
//
// Second byte: X movement.
// Third byte:  Y movement.
//
// Ref: https://wiki.osdev.org/Mouse_Input
static void inline handle_event_mouse_impl(mouse_msg_t msg) {
  // Since bitwise operations below assumes integers are 32 bits, we use i32 to
  // ensure that. "mouse_x" and "mouse_y" mean the offset of mouse movement.
  i32 offset_x = msg.buf[1];
  i32 offset_y = msg.buf[2];

  // Cast pointer to avoid implmentation-defined bit operation on signed
  // integers
  if (msg.buf[0] & 0x10) {
    *(u32 *)&offset_x |= 0xffffff00;
  }
  if (msg.buf[0] & 0x20) {
    *(u32 *)&offset_y |= 0xffffff00;
  }
  
  // Direction of Y-axis of mouse is opposite to that of the screen.
  offset_y = -offset_y;

  int x = g_mouse_layer->x, y = g_mouse_layer->y;
  int new_x = clamp_i32(x + offset_x, 0, g_boot_info.width - 1);
  int new_y = clamp_i32(y + offset_y, 0, g_boot_info.height - 1);

  if (offset_x || offset_y) {
    layer_move_to(g_mouse_layer, new_x, new_y);
  }

  // Check if any layer can receive the mouse event.
  int btn = msg.buf[0];
  decoded_mouse_msg_t decoded_msg = {
    .button = {btn & 0x01, btn & 0x02, btn & 0x04}, // Left/Middle/Right
    .x = x, 
    .y = y,
    .mx = new_x - x, // Use (bounded) new_x - x instead of offset_x
    .my = new_y - y,
    .layer = NULL
  };
  layers_receive_mouse_event(x, y, decoded_msg);
}

static queue_t g_mouse_msg_queue;

#define MOUSE_EVENT_QUEUE_SIZE 512

void init_mouse_event_queue() {
  queue_init(&g_mouse_msg_queue, sizeof(mouse_msg_t), MOUSE_EVENT_QUEUE_SIZE,
             alloc_mem2, reclaim_mem2);
}

void emit_mouse_event(mouse_msg_t msg) {
  queue_push(&g_mouse_msg_queue, &msg);
}

static int mouse_msg_queue_empty() {
  return queue_is_empty(&g_mouse_msg_queue);
}

static void mouse_msg_queue_consume() {
  mouse_msg_t msg;
  queue_pop(&g_mouse_msg_queue, &msg);
  asm_sti();
  handle_event_mouse_impl(msg);
}

event_queue_t g_mouse_event_queue = {
  .empty = mouse_msg_queue_empty,
  .consume = mouse_msg_queue_consume
};
