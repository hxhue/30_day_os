#include <event/timer.h>
#include <boot/def.h>
#include <support/priority_queue.h>
#include <event/event.h>

counter_t g_counter = {.count = 0};

typedef struct timer_t {
  unsigned long long timeout;
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

void add_timer(unsigned long long interval, void (*callback)(void)) {
  timer_t timer = {.timeout = interval + g_counter.count, .callback = callback};
  priority_queue_push(&g_timer_queue, &timer);
}

int timer_event_queue_empty() {
  return priority_queue_is_empty(&g_timer_queue) ||
         ((const timer_t *)priority_queue_get_first(&g_timer_queue))->timeout >
             g_counter.count;
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
