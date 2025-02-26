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

#define MOUSE_EVENT_GET_CONTROL  1 // Not used.
#define MOUSE_EVENT_LOSE_CONTROL 2

struct layer_t;

// Messages sent to processes. More detailed.
typedef struct decoded_mouse_msg_t {
  u8 button[3];          // Left/Middle/Right
  u8 control;            // Not used by now
  int x, y;              // Before this event
  int mx, my;            // Movement in this event
  struct layer_t *layer; // Which layer receives the message?
} decoded_mouse_msg_t;

// extern event_queue_t g_mouse_event_queue;
extern struct layer_t *g_mouse_layer;

// void handle_event_mouse(unsigned data);
void emit_mouse_event(mouse_msg_t msg);
void init_mouse_event_queue();
void init_mouse();

// Check if there is a mouse event. If so, process it and return 0. Otherwise, 1
// is returned. If there're multiple events, only process one of them. asm_cli()
// and asm_sti() are called inside this function.
int check_mouse_events();

#if (defined(__cplusplus))
}
#endif

#endif

