#ifndef KEYBOARD_H
#define KEYBOARD_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>

extern event_queue_t g_keyboard_event_queue;
extern const char g_keycode_table[0x54];

void emit_keyboard_event(unsigned data);
void init_keyboard_event_queue();
void wait_kbdc_ready();
void init_keyboard();

typedef struct keyboard_listener_t {
  void (*on_key_clicked)(int keycode);
  void (*on_key_released)(int keycode);
} keyboard_listener_t;

#if (defined(__cplusplus))
}
#endif

#endif

