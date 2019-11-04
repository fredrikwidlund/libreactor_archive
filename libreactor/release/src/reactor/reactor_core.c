#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <err.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"

typedef struct reactor_core_polls reactor_core_polls;
struct reactor_core_polls
{
  size_t              size;
  size_t              allocated;
  struct pollfd      *pollfd;
  reactor_user       *user;
};

typedef struct reactor_core reactor_core;
struct reactor_core
{
  reactor_core_polls  polls_current;
  reactor_core_polls  polls_next;
  reactor_pool        pool;
};

static __thread struct reactor_core core = {0};

static void reactor_core_resize_polls(size_t size)
{
  reactor_core_polls polls_new;
  size_t i;

  if (reactor_unlikely(size > core.polls_next.allocated))
    {
      while (size > core.polls_next.allocated)
        core.polls_next.allocated = core.polls_next.allocated ? core.polls_next.allocated * 2 : 32;

      polls_new.size = core.polls_next.size;
      polls_new.allocated = core.polls_next.allocated;
      polls_new.pollfd = calloc(polls_new.allocated, sizeof *polls_new.pollfd);
      polls_new.user = calloc(polls_new.allocated, sizeof *polls_new.user);
      memcpy(polls_new.pollfd, core.polls_next.pollfd, core.polls_next.size * sizeof *polls_new.pollfd);
      memcpy(polls_new.user, core.polls_next.user, core.polls_next.size * sizeof *polls_new.user);
      if (core.polls_next.pollfd != core.polls_current.pollfd)
        {
          free(core.polls_next.pollfd);
          free(core.polls_next.user);
        }
      core.polls_next = polls_new;
    }

  for (i = core.polls_next.size; i < size; i ++)
    core.polls_next.pollfd[i].fd = -1;
  core.polls_next.size = size;
}

static void reactor_core_fd_event(reactor_user *user, short *revents, int *boundary)
{
  switch (*revents)
    {
    case 0:
      (*boundary)++;
      break;
    case POLLIN:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_READ, NULL);
      break;
    case POLLOUT:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_WRITE, NULL);
      break;
    case POLLIN | POLLOUT:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_WRITE, NULL);
      if (*revents)
        reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_READ, NULL);
      break;
    case POLLHUP:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_HANGUP, NULL);
      break;
    case POLLIN | POLLHUP:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_READ, NULL);
      if (*revents)
        reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_HANGUP, NULL);
      break;
    default:
      reactor_user_dispatch(user, REACTOR_CORE_FD_EVENT_ERROR, NULL);
      break;
    }
}

void reactor_core_fd_register(int fd, reactor_user_callback *callback, void *state, short events)
{
  size_t size = fd + 1;

  if (size > core.polls_next.size)
    reactor_core_resize_polls(size);
  core.polls_next.pollfd[fd] = (struct pollfd) {.fd = fd, .events = events};
  reactor_user_construct(&core.polls_next.user[fd], callback, state);
}

void reactor_core_fd_deregister(int fd)
{
  if ((size_t) fd < core.polls_current.size)
    core.polls_current.pollfd[fd].revents = 0;
  core.polls_next.pollfd[fd] = (struct pollfd) {.fd = -1};
  while (core.polls_next.size && core.polls_next.pollfd[core.polls_next.size - 1].fd == -1)
    core.polls_next.size --;
}

void reactor_core_fd_set(int fd, short mask)
{
  core.polls_next.pollfd[fd].events |= mask;
}

void reactor_core_fd_clear(int fd, short mask)
{
  core.polls_next.pollfd[fd].events &= ~mask;
}

void reactor_core_job_register(reactor_user_callback *callback, void *state)
{
  reactor_pool_enqueue(&core.pool, callback, state);
}

void reactor_core_construct()
{
  core = (struct reactor_core) {0};
  reactor_pool_construct(&core.pool);
}

void reactor_core_destruct()
{
  reactor_pool_destruct(&core.pool);
  free(core.polls_next.pollfd);
  free(core.polls_next.user);
}

int reactor_core_run(void)
{
  int i, n = 0;

  while (core.polls_next.size)
    {
      if (reactor_unlikely(core.polls_next.pollfd != core.polls_current.pollfd))
        {
          free(core.polls_current.pollfd);
          free(core.polls_current.user);
        }
      core.polls_current = core.polls_next;
      n = poll(core.polls_current.pollfd, core.polls_current.size, -1);
      if (n == -1)
        break;

      for (i = 0; i < n && i < (int) core.polls_current.size; i ++)
        reactor_core_fd_event(&core.polls_current.user[i], &core.polls_current.pollfd[i].revents, &n);
    }

  return n == -1 ? -1 : 0;
}
