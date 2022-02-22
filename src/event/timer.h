#ifndef TIMER_H
#define TIMER_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>

extern event_queue_t g_timer_event_queue;

void emit_timer_event(unsigned data);
void init_timer_event_queue();
void add_timer(unsigned interval, void (*callback)(void));

#if (defined(__cplusplus))
}
#endif

#endif