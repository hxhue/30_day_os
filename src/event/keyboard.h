#ifndef KEYBOARD_H
#define KEYBOARD_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>

extern event_queue_t g_keyboard_event_queue;

void emit_keyboard_event(unsigned data);
void init_keyboard_event_queue();

#if (defined(__cplusplus))
}
#endif

#endif

