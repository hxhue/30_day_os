#include <event/timer.h>
#include <boot/def.h>
#include <support/priority_queue.h>
#include <event/event.h>

// data: number of interrupts happened after the timer is initialized.
void emit_timer_event(unsigned data) {
  // The queue uses global counter information to check if a timer has expired.
  // So this function is empty. It's only reserved for future usage... ?
}

typedef struct timer_t {
  unsigned timeout;
  void (*callback)(void);
} timer_t;

static priority_queue_t g_timer_queue;

void timer_swap(void *a, void *b) {
  timer_t t = *(timer_t *)a;
  *(timer_t *)a = *(timer_t *)b;
  *(timer_t *)b = t;
}

int timer_greater(void *a, void *b) {
  return ((timer_t *)a)->timeout > ((timer_t *)b)->timeout;
}

#define TIMER_QUEUE_SIZE 1024

void init_timer_event_queue() {
  priority_queue_init(&g_timer_queue, sizeof(timer_t), TIMER_QUEUE_SIZE,
                      timer_swap, timer_greater);
}

void add_timer(unsigned interval, void (*callback)(void)) {
  timer_t timer = {.timeout = interval + g_counter.count, .callback = callback};
  priority_queue_push(&g_timer_queue, &timer);
}

int timer_event_queue_empty() {
  if (priority_queue_is_empty(&g_timer_queue))
    return 1;
  
  timer_t timer;
  priority_queue_peek(&g_timer_queue, &timer);
  return timer.timeout > g_counter.count;
}

void timer_event_queue_consume() {
  timer_t timer;
  priority_queue_pop(&g_timer_queue, &timer);
  asm_sti();
  timer.callback();
}

event_queue_t g_timer_event_queue = {
    .empty = timer_event_queue_empty,
    .consume = timer_event_queue_consume
};
