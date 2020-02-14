#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <poll.h>
#include <signal.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/timerfd.h>

#include "dynamic.h"
#include "reactor.h"

static void reactor_timer_hold(reactor_timer *timer)
{
  timer->refs ++;
}

static void reactor_timer_release(reactor_timer *timer)
{
  timer->refs --;
  if (!timer->refs)
    reactor_user_dispatch(&timer->user, REACTOR_TIMER_DESTRUCT, timer);
}

static void reactor_timer_error(reactor_timer *timer)
{
  reactor_user_dispatch(&timer->user, REACTOR_TIMER_ERROR, timer);
}

static void reactor_timer_event(void *state, int type, void *data)
{
  reactor_timer *timer;
  uint64_t expirations;
  ssize_t n;

  (void) data;
  timer = state;
  switch(type)
    {
    case REACTOR_DESC_READ:
      n = read(reactor_desc_fd(&timer->desc), &expirations, sizeof expirations);
      if (n != sizeof expirations)
        break;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_CALL, &expirations);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_desc_close(&timer->desc);
      break;
    case REACTOR_DESC_DESTRUCT:
      reactor_timer_release(timer);
      break;
    }
}

void reactor_timer_construct(reactor_timer *timer, reactor_user_callback *callback, void *state,
                             uint64_t initial, uint64_t interval)
{
  int fd;

  timer->refs = 0;
  timer->flags = REACTOR_TIMER_FLAG_ACTIVE;
  reactor_user_construct(&timer->user, callback, state);
  reactor_timer_hold(timer);

  fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1)
    {
      reactor_timer_error(timer);
      return;
    }

  reactor_timer_hold(timer);
  reactor_desc_construct(&timer->desc, reactor_timer_event, timer, fd, REACTOR_DESC_MASK_READ);
  reactor_timer_set(timer, initial, interval);
}

void reactor_timer_set(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  int e;

  e = timerfd_settime(reactor_desc_fd(&timer->desc), 0, (struct itimerspec[]) {{
        .it_interval = {.tv_sec = interval / 1000000000, .tv_nsec = interval % 1000000000},
        .it_value = {.tv_sec = initial / 1000000000, .tv_nsec = initial % 1000000000}
      }}, NULL);
  if (e == -1)
    reactor_timer_error(timer);
}

int reactor_timer_active(reactor_timer *timer)
{
  return (timer->flags & REACTOR_TIMER_FLAG_ACTIVE) != 0;
}

void reactor_timer_close(reactor_timer *timer)
{
  if (reactor_timer_active(timer))
    {
      timer->flags &= ~REACTOR_TIMER_FLAG_ACTIVE;
      if (reactor_desc_active(&timer->desc))
        reactor_desc_close(&timer->desc);
      reactor_timer_release(timer);
    }
}
