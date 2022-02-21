#ifndef EVENT_H
#define EVENT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

enum Event {
  EVENT_NONE = 0,
  EVENT_KEYBOARD, // Data: port 8-bit data
  EVENT_MOUSE,    // Data: port 8-bit data
  EVENT_REDRAW,   // Data: 0 for full-redrawing, otherwise for region 
                  // [x0,y0,x1,y1] with a factor.
  NUM_EVENT_TYPES
};

typedef struct event_t {
  enum Event type;
  i32 data;
} event_t;

void raise_event(enum Event e, i32 data);
void event_loop();
void prepare_event_loop();

#if (defined(__cplusplus))
}
#endif

#endif

