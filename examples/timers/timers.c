// MIT License
// Copyright (c) 2024 Anotra
// https://github.com/Anotra/anoheap

#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined HAVE_GETTIMEOFDAY
  #include <sys/time.h>
#endif

#include "anoheap.h"

#include "timers.h"

ANOHEAP_DECLARE_COMPARE_FUNCTION(_cmp_timers, uint64_t)

struct timers {
  anoheap *h;
  struct {
    timer_id id;
    bool skip_update;
  } active;
};

static uint64_t
_time_in_millis(void) {
  struct timespec ts;
#if defined HAVE_CLOCK_GETTIME
  #if defined CLOCK_REALTIME_COURSE
    clock_gettime(CLOCK_REALTIME_COURSE, &ts);
  #else
    clock_gettime(CLOCK_REALTIME, &ts);
  #endif
#elif defined HAVE_TIMESPEC_GET
  timespec_get(&ts, TIME_UTC);
#elif defined HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ts.tv_sec = tv.tv_sec;
  ts.tv_nsec = (long)tv.tv_usec * 1000;
#else
  ts.tv_sec = time(NULL);
  ts.tv_nsec = 0;
#endif
  return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct timers *
timers_create(void) {
  struct timers *t = calloc(1, sizeof *t);
  if (!t) return NULL;
  if ((t->h = anoheap_create(sizeof(uint64_t),
                             sizeof(struct timer),
                             _cmp_timers, 0)))
    return t;
  return free(t), NULL;
}

void
timers_destroy(struct timers *t) {
  anoheap_destroy(t->h);
  memset(t, 0, sizeof *t);
  free(t);
}

size_t
timers_active(struct timers *t) {
  return anoheap_enabled_count(t->h);
}

size_t
timers_inactive(struct timers *t) {
  return anoheap_disabled_count(t->h);
}

size_t
timers_run(struct timers *t) {
  struct timer_ev ev = { 0 };
  uint64_t now = _time_in_millis();
  while ((ev.id = anoheap_peek_key(t->h, &ev.trigger))) {
    if (ev.trigger > now) break;
    uint64_t trigger = ev.trigger;
    ev.now = now;
    (ev.timers = t)->active.id = ev.id;
    anoheap_get_val(t->h, ev.id, &ev.timer);
    if (ev.timer.cb) {
      ev.timer.cb(&ev);
      if (t->active.skip_update) {
        t->active.skip_update = false;
        continue;
      }
      if (trigger < ev.trigger) {
        trigger = ev.trigger;
        goto update;
      }
    }
    if (ev.timer.interval) {
      trigger = now + ev.timer.interval;
    } else anoheap_disable(t->h, ev.id);
  update:
    anoheap_update(t->h, ev.id, &trigger, &ev.timer);
  }
  memset(&t->active, 0, sizeof t->active);
  return timers_active(t);
}

uint64_t
timers_next(struct timers *t, uint64_t default_value) {
  uint64_t trigger, now = _time_in_millis();
  return anoheap_peek_key(t->h, &trigger)
    ? trigger <= now ? 0 : trigger - now
    : default_value;
}

timer_id
timers_new(struct timers *t, void (*cb)(struct timer_ev *ev), void *data) {
  struct timer timer = { .cb = cb, .data = data };
  return anoheap_add(t->h, NULL, &timer, false);
}

#define SKIP_UPDATE_PHASE_IF_TIMER_ACTIVE                                     \
  do {                                                                        \
    if (0 == id)  return false;                                               \
    if (t->active.id == id) t->active.skip_update = true;                     \
  } while (0)

bool
timers_delete(struct timers *t, timer_id id) {
  SKIP_UPDATE_PHASE_IF_TIMER_ACTIVE;
  return anoheap_delete(t->h, id);
}

bool
timers_start(struct timers *t, timer_id id,
             uint64_t delay, uint64_t interval)
{
  SKIP_UPDATE_PHASE_IF_TIMER_ACTIVE;
  struct timer *timer = anoheap_get_val_direct(t->h, id);
  if (!timer) return false;
  timer->interval = interval;
  uint64_t trigger = _time_in_millis() + delay;
  anoheap_update_key(t->h, id, &trigger);
  return anoheap_enable(t->h, id);
}

bool
timers_stop(struct timers *t, timer_id id) {
  SKIP_UPDATE_PHASE_IF_TIMER_ACTIVE;
  return anoheap_disable(t->h, id);
}
