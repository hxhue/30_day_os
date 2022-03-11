#ifndef EVENT_H
#define EVENT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>
#include <task/task.h>
#include <support/tree.h>

// Works as an interface type.
// typedef struct event_queue_t {
//   // Checks if the queue is empty. Returns non-zero if empty, and 0 if not.
//   int (*empty)(void);
//   // Takes one element from the queue, then calls asm_sti() to enable
//   // interrupts, then processes the element.
//   void (*consume)(void);
// } event_queue_t;

void init_devices();
void prepare_event_loop();
void event_loop();

// Stored in processes
enum EventNo {
	EVENTNO_KEYBOARD   = 1,
	EVENTNO_MOUSE      = 12,
};

enum EventBit {
	EVENTBIT_KEYBOARD = (1 << 1),
	EVENTBIT_MOUSE    = (1 << 12),
};

#if (defined(__cplusplus))
}
#endif

#endif

