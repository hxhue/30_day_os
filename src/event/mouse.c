#include <boot/boot.h>
#include <event/event.h>
#include <event/mouse.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <support/type.h>
#include <support/debug.h>

extern layer_info_t *g_mouse_layer;

typedef struct mouse_msg_t {
  u8 buf[3];
} mouse_msg_t;

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
  int btn = msg.buf[0] & 0x07;
  // Since bitwise operations below assumes integers are 32 bits, we use i32 to
  // ensure that. "mouse_x" and "mouse_y" mean the offset of mouse movement.
  i32 mouse_x = msg.buf[1];
  i32 mouse_y = msg.buf[2];

  // Cast pointer to avoid implmentation-defined bit operation
  if (msg.buf[0] & 0x10) {
    *(u32 *)&mouse_x |= 0xffffff00;
  }
  if (msg.buf[0] & 0x20) {
    *(u32 *)&mouse_y |= 0xffffff00;
  }
  
  // Direction of Y-axis of mouse is opposite to that of the screen.
  mouse_y = -mouse_y;

  // if (btn & 0x01) buf[0] = 'L'; // Left button down
  // if (btn & 0x02) buf[1] = 'R'; // Center button down
  // if (btn & 0x04) buf[2] = 'C'; // Right button down

  // Those two variables meant offset, while they mean absolute positions now.
  int new_x = clamp_i32(g_mouse_layer->x + mouse_x, 0, g_boot_info.width - 1);
  int new_y = clamp_i32(g_mouse_layer->y + mouse_y, 0, g_boot_info.height - 1);

  if (mouse_x || mouse_y) {
    move_layer_to(g_mouse_layer, new_x, new_y);
  }

  (void)btn; // Silence warning
}

void handle_event_mouse(int data) {
  // Every mouse event has 3 bytes. Since the port is 8 bit, a mouse event will
  // cause 3 interrupts.

  // The first byte a mouse will receive is 0xfa meaning that mouse
  // initialization is ready.
  static int mouse_state = 3;
  static mouse_msg_t msg = {{0}};

  switch(mouse_state) {
    case 0:
      // Move bits:  0x0~0x3
      // Click bits: 0x8~0xf
      if ((data & 0xc8) == 0x08)
        msg.buf[mouse_state++] = data;
      break;
    case 1:
      msg.buf[mouse_state++] = data;
      break;
    case 2:
      msg.buf[2] = data;
      mouse_state = 0;
      handle_event_mouse_impl(msg);
      break;
    case 3:
      if (data == 0xfa)
        mouse_state = 0;
      break;
    default:
      xassert(!"Unreachable");
      break;
  }
}

