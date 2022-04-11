#include <boot/def.h>
#include <event/event.h>
#include <event/timer.h>
#include <memory/memory.h>
#include <support/asm.h>
#include <support/priority_queue.h>
#include <support/queue.h>
#include <task/task.h>

counter_t g_counter = {.count = 0};

void init_counter() {
  // Notify IRQ-0 Cycle change
  asm_out8(PIT_CTRL, 0X34);
  // PIT: 1.193182 MHz.
  // e.g. 0x2e9c means about 10 ms
  // asm_out8(PIT_CNT0, 0x52);
  // asm_out8(PIT_CNT0, 0x09);
  asm_out8(PIT_CNT0, 0xa8);
  asm_out8(PIT_CNT0, 0x04);
}

priority_queue_t g_timer_queue;

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
                      timer_swap, timer_greater, alloc, reclaim);
}

// Requirements: callback != 0
void add_timer(unsigned long long interval, process_node_t *pnode, int data) {
  timer_t timer = {
    .timeout = interval + g_counter.count,
    .pnode = pnode, 
    .data = data
  };
  // u32 eflags = asm_load_eflags();
  asm_cli();
  priority_queue_push(&g_timer_queue, &timer);
  asm_sti();
  // asm_store_eflags(eflags);
}

int timer_event_queue_empty() {
  return priority_queue_is_empty(&g_timer_queue) ||
         ((const timer_t *)priority_queue_get_first(&g_timer_queue))->timeout >
             g_counter.count;
}

// add_timer() cannot be called in a interrupt handler, so asm_sti() can be
// called immediately.
// TODO: move g_timer_event_queue abstraction
// void timer_event_queue_consume() {
//   asm_sti();
//   timer_t timer;
//   priority_queue_pop(&g_timer_queue, &timer);
//   queue_push(&get_proc_from_node(timer.pnode)->timer_msg_queue, &timer.data);
//   process_set_urgent(timer.pnode);
// }

// event_queue_t g_timer_event_queue = {
//     .empty = timer_event_queue_empty,
//     .consume = timer_event_queue_consume
// };

int check_timer_events() {
  asm_cli();
  int queue_empty =
      priority_queue_is_empty(&g_timer_queue) ||
      ((timer_t *)priority_queue_get_first(&g_timer_queue))->timeout >
          g_counter.count;
  asm_sti();
  if (!queue_empty) {
    timer_t timer;
    priority_queue_pop(&g_timer_queue, &timer);
    queue_push(&get_proc_from_node(timer.pnode)->timer_msg_queue, &timer.data);
    process_set_urgent(timer.pnode);
  }

  return queue_empty;
}
