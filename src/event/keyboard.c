#include <graphics/draw.h>
#include <event/keyboard.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/queue.h>

void handle_event_kbd_impl(unsigned keycode) {
  // char buf[8];
  // sprintf(buf, "0X%02X", keycode);
  // fill_rect(RGB_AQUA_DARK, 0, 0, 320, 16);
  // put_string(RGB_WHITE, 0, 0, buf);
}

static queue_t keyboard_msg_queue;

#define KEYBOARD_MSG_QUEUE_SIZE 1024

void init_keyboard_event_queue() {
  queue_init(&keyboard_msg_queue, sizeof(unsigned), KEYBOARD_MSG_QUEUE_SIZE);
}

void emit_keyboard_event(unsigned data) {
  queue_push(&keyboard_msg_queue, &data);
}

static int keyboard_event_queue_empty() {
  return queue_is_empty(&keyboard_msg_queue);
}

static void keyboard_event_queue_consume() {
  unsigned keycode;
  queue_pop(&keyboard_msg_queue, &keycode);
  asm_sti();
  handle_event_kbd_impl(keycode);
}

event_queue_t g_kbd_event_queue = {
  .empty = keyboard_event_queue_empty,
  .consume = keyboard_event_queue_consume
};

