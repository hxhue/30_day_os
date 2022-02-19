#include <graphics/draw.h>
#include <event/keyboard.h>
#include <stdio.h>

void handle_event_kbd(int keycode) {
  char buf[8];
  sprintf(buf, "0X%02X", keycode);
  fill_rect(RGB_AQUA_DARK, 0, 0, 320, 16);
  put_string(RGB_WHITE, 0, 0, buf);
}

