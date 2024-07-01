// MIT License
// Copyright (c) 2024 Anotra
// https://github.com/Anotra/anoheap

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "timers.h"

static void
_cb(struct timer_ev *ev) {
  printf("tick[id=%u] trigger %"PRIu64"ms ago. interval=%"PRIu64". data = %s\n", 
         ev->id, ev->now - ev->trigger, ev->timer.interval, ev->timer.data);
  if (ev->timer.interval) ev->timer.interval--;
  if (0 == ev->timer.interval) timers_delete(ev->timers, ev->id);
}

int
main() {
  struct timers *timers = timers_create();
  for (int i=0; i<1000; i++) {
    timer_id id = timers_new(timers, _cb, "Hello World!");
    timers_start(timers, id, i * 100, i);
  }
  while (timers_run(timers)) {
    uint64_t sleep_time = timers_next(timers, 0);
    printf("sleeping for %"PRIu64" ms\n", sleep_time);
    if (sleep_time)
      nanosleep(&(struct timespec) {
                  .tv_sec = sleep_time / 1000,
                  .tv_nsec = (sleep_time % 1000) * 1000000
                }, NULL);
  }
  timers_destroy(timers);
}
