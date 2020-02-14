#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <err.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

typedef struct reactor_core reactor_core;
struct reactor_core
{
  size_t ref;
  int    state;
  int    current;
  vector polls;
  vector users;
};

static __thread struct reactor_core core = {.ref = 0};

static void reactor_core_incref(reactor_core *core)
{
  core->ref ++;
}

static void reactor_core_decref(reactor_core *core)
{
  core->ref --;
}

static int reactor_core_grow(size_t size)
{
  size_t current, i;

  current = vector_size(&core.polls);
  if (size <= current)
    return 0;

  vector_reserve(&core.polls, size);
  vector_reserve(&core.users, size);
  for (i = current; i < size; i ++)
    {
      vector_push_back(&core.polls, (struct pollfd[]) {{.fd = -1}});
      vector_push_back(&core.users, (reactor_user[]) {{0}});
    }

  return 0;
}

void reactor_core_construct()
{
  if (!core.ref)
    {
      vector_construct(&core.polls, sizeof(struct pollfd));
      vector_construct(&core.users, sizeof(reactor_user));
    }
  reactor_core_incref(&core);
}

void reactor_core_destruct()
{
  reactor_core_decref(&core);
  if (!core.ref)
    {
      vector_destruct(&core.polls);
      vector_destruct(&core.users);
    }
}

int reactor_core_add(int fd, reactor_user_callback *callback, void *state, int events)
{
  int e;

  e = reactor_core_grow(fd + 1);
  if (e == -1)
    return -1;

  *(struct pollfd *) vector_at(&core.polls, fd) = (struct pollfd) {.fd = fd, .events = events};
  *(reactor_user *) vector_at(&core.users, fd) = (reactor_user) {.callback = callback, .state = state};
  return 0;
}

void reactor_core_remove(int fd)
{
  *(struct pollfd *) vector_at(&core.polls, fd) = (struct pollfd) {.fd = -1};
  *(reactor_user *) vector_at(&core.users, fd) = (reactor_user) {0};

  while (vector_size(&core.polls) && ((struct pollfd *) vector_back(&core.polls))->fd == -1)
    {
      vector_pop_back(&core.polls);
      vector_pop_back(&core.users);
    }
}

int reactor_core_get(int fd)
{
  return ((struct pollfd *) vector_at(&core.polls, fd))->revents;
}

void reactor_core_set(int fd, int events)
{
  ((struct pollfd *) vector_at(&core.polls, fd))->events |= events;
}

void reactor_core_clear(int fd, int events)
{
  ((struct pollfd *) vector_at(&core.polls, fd))->events &= ~events;
}

int reactor_core_run(void)
{
  struct pollfd *pollfd;
  size_t i;
  int e;

  core.state = REACTOR_CORE_ACTIVE;
  while (core.state == REACTOR_CORE_ACTIVE && vector_size(&core.polls))
    {
      e = poll(vector_data(&core.polls), vector_size(&core.polls), -1);
      if (e == -1)
        return -1;

      for (i = 0; i < vector_size(&core.polls); i ++)
        {
          pollfd = vector_at(&core.polls, i);
          if (pollfd->revents)
            reactor_user_dispatch(vector_at(&core.users, i), REACTOR_CORE_POLL, &pollfd->revents);
        }
    }

  return 0;
}
