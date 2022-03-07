#ifndef TIMER_H
#define TIMER_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>
#include <support/priority_queue.h>

typedef struct counter_t {
  unsigned long long count;
} counter_t;

typedef struct timer_t {
  unsigned long long timeout;
  process_node_t *pnode;
  int data;
} timer_t;

extern counter_t g_counter;
extern priority_queue_t g_timer_queue;
extern event_queue_t g_timer_event_queue;

void init_timer_event_queue();
void add_timer(unsigned long long interval, process_node_t *pnode, int data);
void init_counter();

#if (defined(__cplusplus))
}
#endif

#endif
