#ifndef EVENT_H
#define EVENT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

// typedef enum Event {
//   EVENT_NONE = 0,
//   EVENT_KEYBOARD, // Data: port 8-bit data
//   EVENT_MOUSE,    // Data: port 8-bit data
//   EVENT_REDRAW,   // Data: 0 for full-redrawing, otherwise for region 
//                   // [x0,y0,x1,y1] with a factor.
//   EVENT_COUNTER,
//   NUM_EVENT_TYPES,
// } Event;

// typedef struct event_t {
//   enum Event type;
//   u32 data;
// } event_t;

// void raise_event(enum Event e, u32 data);

// Works like an interface type.
typedef struct event_queue_t {
  // Checks if the queue is empty. Returns non-zero if empty, and 0 if not.
  int (*empty)(void);
  // Takes exactly one element from the queue, then calls asm_sti() to enable
  // interrupts, then processes the element.
  void (*consume)(void);
} event_queue_t;

void init_devices();
void prepare_event_loop();
_Noreturn void event_loop();

#if (defined(__cplusplus))
}
#endif

#endif

