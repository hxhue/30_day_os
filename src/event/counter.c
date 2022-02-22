#include <event/counter.h>
#include <boot/def.h>

timer_t g_timer = {.count = 0};

void handle_event_counter(unsigned data) {
  g_timer.count++;
}

void emit_counter_event(unsigned data) {
  handle_event_counter(data);
}

