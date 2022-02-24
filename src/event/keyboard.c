#include "memory/memory.h"
#include <graphics/draw.h>
#include <event/keyboard.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/queue.h>

// static keyboard_listener_queue;

void add_keyboard_listener(keyboard_listener_t listener) {

}

// 0 means the keycode is reserved or not used.
// Based on English keyboard.
const char g_keycode_table[0x54] = {
  0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
  'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0,   ']', 'Z', 'X', 'C', 'V',
  'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
  '2', '3', '0', '.'
};

void handle_event_keyboard_impl(unsigned keycode) {
  // char buf[8];
  // sprintf(buf, "0X%02X", keycode);
  // fill_rect(RGB_AQUA_DARK, 0, 0, 320, 16);
  // put_string(RGB_WHITE, 0, 0, buf);
}

static queue_t keyboard_msg_queue;

#define KEYBOARD_MSG_QUEUE_SIZE 512

void init_keyboard_event_queue() {
  queue_init(&keyboard_msg_queue, sizeof(unsigned), KEYBOARD_MSG_QUEUE_SIZE,
             alloc_mem, reclaim_mem_no_return_value);
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
  handle_event_keyboard_impl(keycode);
}

event_queue_t g_keyboard_event_queue = {
  .empty = keyboard_event_queue_empty,
  .consume = keyboard_event_queue_consume
};

