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

#include "dynamic.h"
#include "reactor.h"

static void reactor_async_hold(reactor_async *async)
{
  async->refs ++;
}

static void reactor_async_release(reactor_async *async)
{
  async->refs --;
  if (!async->refs)
    reactor_user_dispatch(&async->user, REACTOR_ASYNC_DESTRUCT, async);
}

static void reactor_async_event(void *state, int type, void *data)
{
  reactor_async *async;

  (void) data;
  async = state;
  switch(type)
    {
    case REACTOR_DESC_CLOSE:
      free(async->stack);
      async->flags &= ~REACTOR_ASYNC_FLAG_CLONE;
      reactor_user_dispatch(&async->user, REACTOR_ASYNC_CLOSE, async);
      break;
    case REACTOR_DESC_DESTRUCT:
      reactor_async_release(async);
      break;
    }
}

static int reactor_async_clone(void *arg)
{
  reactor_async *async;

  async = arg;
  reactor_user_dispatch(&async->user, REACTOR_ASYNC_CALL, async);
  return 0;
}

static int reactor_async_call(reactor_async *async)
{
  int e;

  async->stack = malloc(REACTOR_ASYNC_STACK_SIZE);
  if (!async->stack)
    return -1;

  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, async->fd);
  if (e == -1)
    {
      free(async->stack);
      return -1;
    }

  async->pid = clone(reactor_async_clone, (char *) async->stack + REACTOR_ASYNC_STACK_SIZE,
                     CLONE_VM | CLONE_FS | CLONE_SIGHAND | CLONE_SYSVSEM | CLONE_PARENT,
                     async);
  if (async->pid == -1)
    {
      free(async->stack);
      (void) close(async->fd[0]);
      (void) close(async->fd[1]);
      return -1;
    }

  async->flags &= REACTOR_ASYNC_FLAG_CLONE;
  (void) close(async->fd[0]);
  reactor_async_hold(async);
  reactor_desc_construct(&async->desc, reactor_async_event, async, async->fd[1], 0);
  return 0;
}

void reactor_async_construct(reactor_async *async, reactor_user_callback *callback, void *state)
{
  int e;

  *async = (reactor_async) {.flags = REACTOR_ASYNC_FLAG_ACTIVE};
  reactor_user_construct(&async->user, callback, state);
  reactor_async_hold(async);
  e = reactor_async_call(async);
  if (e == -1)
    reactor_user_dispatch(&async->user, REACTOR_ASYNC_ERROR, async);
}

int reactor_async_active(reactor_async *async)
{
  return (async->flags & REACTOR_ASYNC_FLAG_ACTIVE) != 0;
}

void reactor_async_close(reactor_async *async)
{
  async->flags &= ~REACTOR_ASYNC_FLAG_ACTIVE;
  if (async->flags & REACTOR_ASYNC_FLAG_CLONE)
    {
      (void) kill(async->pid, SIGKILL);
      async->flags &= ~REACTOR_ASYNC_FLAG_CLONE;
    }

  if (reactor_desc_active(&async->desc))
    {
      reactor_desc_close(&async->desc);
      reactor_async_release(async);
    }

  reactor_async_release(async);
}
