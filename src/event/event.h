#ifndef EVENT_H
#define EVENT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

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
void event_loop();

#if (defined(__cplusplus))
}
#endif

#endif

