#ifndef MOUSE_H
#define MOUSE_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <event/event.h>

typedef struct mouse_msg_t {
  u8 buf[3];
} mouse_msg_t;

extern event_queue_t g_mouse_event_queue;

// void handle_event_mouse(unsigned data);
void emit_mouse_event(mouse_msg_t msg);
void init_mouse_event_queue();
void init_mouse();

#if (defined(__cplusplus))
}
#endif

#endif

