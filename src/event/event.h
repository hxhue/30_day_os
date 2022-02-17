#ifndef EVENT_H
#define EVENT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <type.h>

enum Event {
    EVENT_NONE = 0,
    EVENT_KEYBOARD,
    EVENT_MOUSE,
    EVENT_REDRAW,
    NUM_EVENT_TYPES
};

typedef struct event_t {
  enum Event type;
  u32 data;
} event_t;

// TODO: Set flags if queue overflows
// Book P.187
void raise_event(event_t e);
void event_loop();
void prepare_event_loop();

#if (defined(__cplusplus))
}
#endif

#endif

