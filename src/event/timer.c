#include "memory/memory.h"
#include <event/timer.h>
#include <boot/def.h>
#include <support/priority_queue.h>
#include <event/event.h>

counter_t g_counter = {.count = 0};

void init_counter() {
  // Notify IRQ-0 Cycle change
  asm_out8(PIT_CTRL, 0X34);
  // PIT: 1.193182 MHz.
  // 0x2e9c -> about 10 ms, 0x0952 -> about 2 ms.
  asm_out8(PIT_CNT0, 0x52);
  asm_out8(PIT_CNT0, 0x09);
  // asm_out8(PIT_CNT0, 0x9c);
  // asm_out8(PIT_CNT0, 0x2e);
}

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

#define TIMER_QUEUE_SIZE 2048

void init_timer_event_queue() {
  priority_queue_init(&g_timer_queue, sizeof(timer_t), TIMER_QUEUE_SIZE,
                      timer_swap, timer_greater, alloc_mem2, reclaim_mem2);
}

// Requirements: callback != 0
void add_timer(unsigned long long interval, void (*callback)(void)) {
  timer_t timer = {.timeout = interval + g_counter.count, .callback = callback};
  priority_queue_push(&g_timer_queue, &timer);
}

int timer_event_queue_empty() {
  return priority_queue_is_empty(&g_timer_queue) ||
         ((const timer_t *)priority_queue_get_first(&g_timer_queue))->timeout >
             g_counter.count;
}

// add_timer() cannot be called in a interrupt handler, so asm_sti() can be
// called immediately.
void timer_event_queue_consume() {
  asm_sti();
  timer_t timer;
  priority_queue_pop(&g_timer_queue, &timer);
  timer.callback();
}

event_queue_t g_timer_event_queue = {
    .empty = timer_event_queue_empty,
    .consume = timer_event_queue_consume
};
