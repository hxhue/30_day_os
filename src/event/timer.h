#ifndef TIMER_H
#define TIMER_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>

typedef struct counter_t {
  unsigned long long count;
} counter_t;

extern counter_t g_counter;
extern event_queue_t g_timer_event_queue;

void init_timer_event_queue();
void add_timer(unsigned long long interval, void (*callback)(void));
void init_counter();

#if (defined(__cplusplus))
}
#endif

#endif
