#include <stdlib.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_memory.h"
#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_stream.h"

static void reactor_stream_close_fd(reactor_stream *stream)
{
  if (stream->fd >= 0)
    {
      reactor_core_fd_deregister(stream->fd);
      (void) close(stream->fd);
      stream->fd = -1;
    }
}

static void reactor_stream_error(reactor_stream *stream)
{
  stream->state = REACTOR_STREAM_STATE_ERROR;
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_ERROR, NULL);
}

static void reactor_stream_read(reactor_stream *stream)
{
  reactor_memory memory;
  char block[REACTOR_STREAM_BLOCK_SIZE];
  ssize_t n;

  n = read(stream->fd, block, sizeof block);
  if (reactor_unlikely(n <= 0))
    {
      if (n == 0)
        {
          reactor_stream_close_fd(stream);
          reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_HANGUP, NULL);
        }
      else if (errno != EAGAIN)
        reactor_stream_error(stream);
      return;
    }

  stream->consumed = 0;
  if (reactor_likely(!buffer_size(&stream->input)))
    {
      memory = reactor_memory_ref(block, n);
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &memory);
      if (reactor_unlikely(stream->consumed < (size_t) n))
        buffer_insert(&stream->input, buffer_size(&stream->input), block + stream->consumed, n - stream->consumed);
    }
  else
    {
      buffer_insert(&stream->input, buffer_size(&stream->input), block, n);
      memory = reactor_memory_ref(buffer_data(&stream->input), buffer_size(&stream->input));
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &memory);
      buffer_erase(&stream->input, 0, stream->consumed);
    }
}

#ifndef GCOV_BUILD
static
#endif
void reactor_stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      reactor_stream_read(stream);
      break;
    case REACTOR_CORE_FD_EVENT_WRITE:
      reactor_stream_flush(stream);
      break;
    case REACTOR_CORE_FD_EVENT_HANGUP:
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_HANGUP, NULL);
      break;
    case REACTOR_CORE_FD_EVENT_ERROR:
      reactor_stream_error(stream);
      break;
    }
}

void reactor_stream_hold(reactor_stream *stream)
{
  stream->ref ++;
}

void reactor_stream_release(reactor_stream *stream)
{
  stream->ref --;
  if (reactor_unlikely(!stream->ref))
    {
      reactor_stream_close_fd(stream);
      buffer_destruct(&stream->input);
      buffer_destruct(&stream->output);
      stream->state = REACTOR_STREAM_STATE_CLOSED;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_CLOSE, NULL);
    }
}

void reactor_stream_open(reactor_stream *stream, reactor_user_callback *callback, void *state, int fd)
{
  int e;

  stream->ref = 0;
  stream->state = REACTOR_STREAM_STATE_OPEN;
  reactor_user_construct(&stream->user, callback, state);
  stream->flags = 0;
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
  reactor_stream_hold(stream);
  stream->fd = fd;
  if (stream->fd < 0)
    {
      reactor_stream_error(stream);
      return;
    }

  reactor_core_fd_register(stream->fd, reactor_stream_event, stream, REACTOR_CORE_FD_MASK_READ);
  e = fcntl(stream->fd, F_SETFL, O_NONBLOCK);
  if (e == -1)
    reactor_stream_error(stream);
}

void reactor_stream_close(reactor_stream *stream)
{
  switch (stream->state)
    {
    case REACTOR_STREAM_STATE_OPEN:
      stream->state = REACTOR_STREAM_STATE_CLOSING;
      reactor_stream_flush(stream);
      break;
    case REACTOR_STREAM_STATE_ERROR:
      reactor_stream_release(stream);
      break;
    }
}

void reactor_stream_write(reactor_stream *stream, void *data, size_t size)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), data, size);
}

void reactor_stream_flush(reactor_stream *stream)
{
  ssize_t n;

  if (stream->state & ~(REACTOR_STREAM_STATE_OPEN | REACTOR_STREAM_STATE_CLOSING))
    return;

  if (buffer_size(&stream->output))
    {
      n = write(stream->fd, buffer_data(&stream->output), buffer_size(&stream->output));
      if (reactor_unlikely(n == -1))
        {
          if (errno == EAGAIN)
            {
              reactor_core_fd_set(stream->fd, REACTOR_CORE_FD_MASK_WRITE);
              stream->flags |= REACTOR_STREAM_FLAG_BLOCKED;
              reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_WRITE_BLOCKED, NULL);
            }
          else
            reactor_stream_error(stream);
          return;
        }
      buffer_erase(&stream->output, 0, n);
    }

  if (reactor_unlikely(buffer_size(&stream->output)))
    reactor_core_fd_set(stream->fd, REACTOR_CORE_FD_MASK_WRITE);
  else if (stream->state == REACTOR_STREAM_STATE_CLOSING)
    reactor_stream_release(stream);
  else if (stream->flags & REACTOR_STREAM_FLAG_BLOCKED)
    {
      reactor_core_fd_clear(stream->fd, REACTOR_CORE_FD_MASK_WRITE);
      stream->flags &= ~REACTOR_STREAM_FLAG_BLOCKED;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_WRITE_UNBLOCKED, NULL);
    }
}

void reactor_stream_consume(reactor_stream *stream, size_t size)
{
  stream->consumed += size;
}
