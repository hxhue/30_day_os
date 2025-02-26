#include "event/event.h"
#include "memory/memory.h"
#include "support/debug.h"
#include "support/list.h"
#include "support/tree.h"
#include "task/task.h"
#include <boot/def.h>
#include <event/keyboard.h>
#include <graphics/draw.h>
#include <stdio.h>
#include <support/asm.h>
#include <support/queue.h>

#define KEYCODE_ESCAPE     0X01
#define KEYCODE_BACKSPACE  0X0E
#define KEYCODE_TAB        0X0F
#define KEYCODE_ENTER      0X1C
#define KEYCODE_LEFTCTRL   0X1D
#define KEYCODE_LEFTSHIFT  0X2A
#define KEYCODE_RIGHTSHIFT 0X36
#define KEYCODE_LEFTALT    0X38
#define KEYCODE_CAPSLOCK   0X3A
#define KEYCODE_F1         0X3B
#define KEYCODE_F2         0X3C
#define KEYCODE_F3         0X3D
#define KEYCODE_F4         0X3E
#define KEYCODE_F5         0X3F
#define KEYCODE_F6         0X40
#define KEYCODE_F7         0X41
#define KEYCODE_F8         0X42
#define KEYCODE_F9         0X43
#define KEYCODE_F10        0X44
#define KEYCODE_NUMLOCK    0X45
#define KEYCODE_SCROLLLOCK 0X46
#define KEYCODE_F11        0X57
#define KEYCODE_F12        0X58

#define KEYCODE_EXTEND     0XE0 // Has another keycode
#define KEYCODE_EXTEND2    0XE1 // Has 2 more keycodes
#define KEYCODE_RIGHTALT   0X38
#define KEYCODE_RIGHTCTRL  0X1D
#define KEYCODE_INSERT     0X52
#define KEYCODE_DELETE     0X53
#define KEYCODE_LEFTARROW  0X4B
#define KEYCODE_HOME       0X47
#define KEYCODE_END        0X4F
#define KEYCODE_UPARROW    0X48
#define KEYCODE_DOWNARROW  0X50
#define KEYCODE_PAGEUP     0X49
#define KEYCODE_PAGEDOWN   0X51
#define KEYCODE_RIGHTARROW 0X4D
#define KEYCODE_KP_SLASH   0X35 // KP is for keypad
#define KEYCODE_KP_ENTER   0X1C
#define KEYCODE_PAUSE_1    0X1D
#define KEYCODE_PAUSE_2    0X45
#define KEYCODE_PAUSE_3    0X9D
#define KEYCODE_PAUSE_4    0XC5
#define KEYCODE_PRTSC_1    0X2A
#define KEYCODE_PRTSC_2    0X37
#define KEYCODE_PRTSC_RELEASE_1 0xB7
#define KEYCODE_PRTSC_RELEASE_2 0xAA

static const char keycode_pause_arr6[] = {0XE1, 0X1D, 0X45, 0XE1, 0X9D, 0XC5};
static const char keycode_prtsc_arr4[] = {0xE0, 0X2A, 0xE0, 0X37};
static const char keycode_prtsc_release_arr4[] = {0xE0, 0xB7, 0xE0, 0xAA};

// PAUSE doesn't have a RELEASE version.
// PRTSC's RELEASE version is quite different.

// 0 means the keycode is reserved or not used.
// Based on English keyboard (US QWERTY, Scan Code Set 1).
// See http://osdev.foofun.cn/index.php?title=PS/2_Keyboard
const char g_keycode_table[0x54] = {
    0, 0,    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
    0, 0,    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
    0, 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,    '*',
    0, ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0, '7',  '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.'};

const char g_keycode_shift_table[0x54] = {
    0, 0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',  '+',
    0, 0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{',  '}',
    0, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,    '*',
    0, ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0',  '.'};

// void handle_event_keyboard_impl(unsigned keycode) {
//   // TODO: transfer data to process of focused window
// }

// TODO: remove keyboard_msg_queue
// static queue_t keyboard_msg_queue;

// #define KEYBOARD_MSG_QUEUE_SIZE 512

// void init_keyboard_event_queue() {
//   queue_init(&keyboard_msg_queue, sizeof(unsigned), KEYBOARD_MSG_QUEUE_SIZE,
//              alloc_mem2, reclaim_mem2);
// }

// void emit_keyboard_event(unsigned data) {
//   queue_push(&keyboard_msg_queue, &data);
// }

// static int keyboard_event_queue_empty() {
//   return queue_is_empty(&keyboard_msg_queue);
// }

// static void keyboard_event_queue_consume() {
//   unsigned keycode;
//   queue_pop(&keyboard_msg_queue, &keycode);
//   asm_sti();
//   handle_event_keyboard_impl(keycode);
// }

// event_queue_t g_keyboard_event_queue = {.empty = keyboard_event_queue_empty,
//                                         .consume =
//                                             keyboard_event_queue_consume};

// Keyboard controller is slow so CPU has to wait.
// But kbdc was fast when I tested it in Qemu.
void wait_kbdc_ready() {
  while (asm_in8(PORT_KEYSTA) & KEYSTA_SEND_NOT_READY) {
    // Continue
  }
}

void init_keyboard() {
  wait_kbdc_ready();
  // Send command: set mode (0x60).
  asm_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_kbdc_ready();
  // Send data: a mode that can use mouse (0x47).
  // Mouse communicates through keyboard control circuit.
  asm_out8(PORT_KEYDAT, KBC_MODE);
}

// Qemu doesn't seem to generate key events for PrtSc and Right shift.
// The generated Pause keycode sequence is 0XE0, 0X22. (Strange!)
// TODO: PrtSc not working
// TODO: Right shift not working
// TODO: Pause not working

// TODO: Control/Alt等修饰按键

int process_keycode(int keycode) {
  static int keycode_mode = 0;
  static int keycode_offset = 0;
  static int status_capital = 0;
  static int status_left_shift = 0;
  static int status_right_shift = 0;

  int key = 0;

  // xprintf("keycode: 0X%02X\n", keycode);

  switch (keycode_mode) {
  case 0: {
    if (keycode == KEYCODE_EXTEND || keycode == KEYCODE_EXTEND2) {
      keycode_mode = keycode;
      keycode_offset = 0;
    } else {
      if (keycode & 0x80) {
        key |= KEY_RELEASE_FLAG;
        keycode &= 0x7f;
      } else if (keycode == KEYCODE_CAPSLOCK) { // Press mode + capslock
        status_capital = !status_capital;
      }

      if (keycode == KEYCODE_LEFTSHIFT) {
        status_left_shift = !(key & KEY_RELEASE_FLAG);
      } else if (keycode == KEYCODE_RIGHTSHIFT) {
        status_right_shift = !(key & KEY_RELEASE_FLAG);
      }

      key |= keycode;
    }
    break;
  }
  case KEYCODE_EXTEND: {
    if (keycode == KEYCODE_PRTSC_1 || keycode == KEYCODE_PRTSC_RELEASE_1) {
      keycode_mode = keycode;
      keycode_offset++;
    } else {
      if (keycode & 0x80) {
        key |= KEY_RELEASE_FLAG;
        keycode &= 0x7f;
      }
      key |= keycode;
      key |= 0XE000;
      keycode_mode = 0;
    }
    break;
  }
  case KEYCODE_EXTEND2: {
    if (keycode == KEYCODE_PAUSE_1) {
      keycode_mode = keycode;
      keycode_offset++;
    } else {
      xassert(!"Unknown keycode");
    }
    break;
  }
  case KEYCODE_PRTSC_1: {
    // xprintf("Working");
    if (keycode == keycode_prtsc_arr4[keycode_offset]) {
      keycode_offset++;
      if (keycode_offset >= 4) {
        key = KEY_PRTSC; 
        keycode_mode = 0;
      }
    } else {
      xassert(!"Unknown keycode");
    }
    break;
  }
  case KEYCODE_PRTSC_RELEASE_1: {
    if (keycode == keycode_prtsc_release_arr4[keycode_offset]) {
      keycode_offset++;
      if (keycode_offset >= 4) {
        key = KEY_PRTSC | KEY_RELEASE_FLAG;
        keycode_mode = 0;
      }
    } else {
      xassert(!"Unknown keycode");
    }
    break;
  }
  case KEYCODE_PAUSE_1: {
    if (keycode == keycode_pause_arr6[keycode_offset]) {
      keycode_offset++;
      if(keycode_offset >= 4) {
        key = KEY_PAUSE;
        keycode_mode = 0;
      }
    } else {
      xassert(!"Unknown keycode");
    }
    break;
  }
  default:
    xassert(!"Unreachable");
  }

  if (key) {
    if (status_capital)
      key |= KEY_CAPITAL_FLAG;
    if (status_left_shift || status_right_shift)
      key |= KEY_SHIFT_FLAG;
  }
    
  return key;
}

static inline int isalpha(int ch) {
  return (ch >= 'a' && ch < 'z') || (ch >= 'A' && ch < 'Z');
}

static inline char toggle_case(char ch) {
  if (ch >= 'a' && ch < 'z')
    ch += 'A' - 'a';
  else if (ch >= 'A' && ch < 'Z')
    ch += 'a' - 'A';
  return ch;
}

char to_plain_char(int key) {
  int i = key & 0x00ffffff;
  char ch = 0;
  if (i >= 0 && i < 0x54) {
    ch = (key & KEY_SHIFT_FLAG) ? g_keycode_shift_table[i] : g_keycode_table[i];
    if (isalpha(ch) && (key & KEY_CAPITAL_FLAG)) {
      ch = toggle_case(ch);
    }
    // if (ch >= 'a' && ch < 'z') {
    //   int capitalize = !!(key & KEY_CAPITAL_FLAG) ^ !!(key & KEY_SHIFT_FLAG);
    //   if (capitalize)
    //     ch += ('A' - 'a');
    // } else {
    //   if (key & KEY_SHIFT_FLAG) {
    //     ch = g_keycode_shift_table[ch];
    //   }
    // }
  }
  return ch;
}
