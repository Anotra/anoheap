// MIT License
// Copyright (c) 2024 Anotra
// https://github.com/Anotra/anoheap

#pragma once

#ifndef ANOHEAP_EXAMPLES_TIMERS_H
#define ANOHEAP_EXAMPLES_TIMERS_H

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned timer_id;

struct timers;

struct timer_ev;

struct timer {
  void *data;
  void (*cb)(struct timer_ev *ev);
  uint64_t interval;
};

struct timer_ev {
  struct timers *timers;
  timer_id id;
  uint64_t trigger;
  uint64_t now;
  struct timer timer;
};

struct timers *timers_create(void);
void timers_destroy(struct timers *t);

/** @return size_t number of active timers */
size_t timers_active(struct timers *t);
/** @return size_t number of inactive timers */
size_t timers_inactive(struct timers *t);

/** runs timers 
 * @return size_t number of active timers */
size_t timers_run(struct timers *t);
/** @return uint64_t time until next timer, or default_value */
uint64_t timers_next(struct timers *t, uint64_t default_value);

timer_id timers_new(struct timers *t,
                    void (*cb)(struct timer_ev *ev),
                    void *data);
bool timers_delete(struct timers *t, timer_id id);

bool timers_start(struct timers *t, timer_id id,
                  uint64_t delay, uint64_t interval);
bool timers_stop(struct timers *t, timer_id id);

#ifdef __cplusplus
}
#endif

#endif // !ANOHEAP_EXAMPLES_TIMERS_H
