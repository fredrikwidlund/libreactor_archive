#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

static void reactor_desc_hold(reactor_desc *desc)
{
  desc->refs ++;
}

static void reactor_desc_release(reactor_desc *desc)
{
  desc->refs --;
  if (!desc->refs)
    reactor_user_dispatch(&desc->user, REACTOR_DESC_DESTRUCT, desc);
}

void reactor_desc_construct(reactor_desc *desc, reactor_user_callback *callback, void *state, int fd, int mask)
{
  int e;

  desc->refs = 0;
  desc->flags = REACTOR_DESC_FLAG_ACTIVE;
  reactor_user_construct(&desc->user, callback, state);
  desc->fd = fd;
  reactor_desc_hold(desc);
  e = reactor_core_add(fd, reactor_desc_event, desc, mask);
  if (e == -1)
    reactor_desc_error(desc);
}

int reactor_desc_active(reactor_desc *desc)
{
  return (desc->flags & REACTOR_DESC_FLAG_ACTIVE) != 0;
}

int reactor_desc_close_take(reactor_desc *desc)
{
  int fd;

  fd = desc->fd;
  reactor_core_remove(desc->fd);
  desc->flags &= ~REACTOR_DESC_FLAG_ACTIVE;
  reactor_desc_release(desc);
  return fd;
}

void reactor_desc_close(reactor_desc *desc)
{
  (void) close(reactor_desc_close_take(desc));
}

void reactor_desc_set(reactor_desc *desc, int flags)
{
  reactor_core_set(desc->fd, flags);
}

void reactor_desc_clear(reactor_desc *desc, int flags)
{
  reactor_core_clear(desc->fd, flags);
}

int reactor_desc_fd(reactor_desc *desc)
{
  return desc->fd;
}

void reactor_desc_error(reactor_desc *desc)
{
  reactor_user_dispatch(&desc->user, REACTOR_DESC_ERROR, desc);
}

void reactor_desc_event(void *state, int type, void *data)
{
  reactor_desc *desc;
  int revents;

  desc = state;
  (void) type;
  revents = *(short int *)data;
  reactor_desc_hold(desc);
  if (reactor_desc_active(desc) && revents & POLLHUP)
    reactor_user_dispatch(&desc->user, REACTOR_DESC_CLOSE, desc);
  if (reactor_desc_active(desc) && revents & (POLLERR | POLLNVAL))
    reactor_user_dispatch(&desc->user, REACTOR_DESC_ERROR, desc);
  if (reactor_desc_active(desc) && revents & POLLOUT)
    reactor_user_dispatch(&desc->user, REACTOR_DESC_WRITE, desc);
  if (reactor_desc_active(desc) && revents & POLLIN)
      reactor_user_dispatch(&desc->user, REACTOR_DESC_READ, desc);
  reactor_desc_release(desc);
}
