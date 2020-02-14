#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

static void reactor_stream_hold(reactor_stream *stream)
{
  stream->refs ++;
}

static void reactor_stream_release(reactor_stream *stream)
{
  stream->refs --;
  if (!stream->refs)
    {
      buffer_destruct(&stream->read);
      buffer_destruct(&stream->write);
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_DESTRUCT, stream);
    }
}

static void reactor_stream_read(reactor_stream *stream)
{
  char buffer[REACTOR_STREAM_BLOCK_SIZE];
  reactor_stream_data data;
  ssize_t n;

  n = read(reactor_desc_fd(&stream->desc), buffer, sizeof buffer);
  if (n <= 0)
    {
      if (n == 0)
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_CLOSE, stream);
      else if (errno != EAGAIN)
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_ERROR, stream);
      return;
    }

  reactor_stream_hold(stream);
  if (buffer_size(&stream->read) == 0)
    {
      data = (reactor_stream_data) {.base = buffer, .size = n};
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_READ, &data);
      if (data.size)
        buffer_insert(&stream->read, buffer_size(&stream->read), data.base, data.size);
    }
  else
    {
      buffer_insert(&stream->read, buffer_size(&stream->read), buffer, n);
      data = (reactor_stream_data) {.base = buffer_data(&stream->read), .size = buffer_size(&stream->read)};
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_READ, &data);
      buffer_erase(&stream->read, 0, buffer_size(&stream->read) - data.size);
    }
  reactor_stream_release(stream);
}

static void reactor_stream_event(void *state, int type, void *data)
{
  reactor_stream *stream;

  stream = state;
  (void) data;
  reactor_stream_hold(stream);
  switch(type)
    {
    case REACTOR_DESC_READ:
      reactor_stream_read(stream);
      break;
    case REACTOR_DESC_WRITE:
      reactor_stream_clear_blocked(stream);
      reactor_stream_flush(stream);
      if (!reactor_stream_blocked(stream))
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_WRITE, stream);
      break;
    case REACTOR_DESC_DESTRUCT:
      reactor_stream_release(stream);
      break;
    }
  reactor_stream_release(stream);
}

void reactor_stream_construct(reactor_stream *stream, reactor_user_callback *callback, void *state, int fd)
{
  *stream = (reactor_stream) {.flags = REACTOR_STREAM_FLAG_ACTIVE};
  reactor_stream_hold(stream);
  reactor_user_construct(&stream->user, callback, state);
  reactor_desc_construct(&stream->desc, reactor_stream_event, stream, fd, REACTOR_DESC_READ);
  buffer_construct(&stream->read);
  buffer_construct(&stream->write);
}

int reactor_stream_active(reactor_stream *stream)
{
  return (stream->flags & REACTOR_STREAM_FLAG_ACTIVE) != 0;
}

void reactor_stream_close(reactor_stream *stream)
{
  stream->flags |= REACTOR_STREAM_FLAG_LINGER;
  stream->flags &= ~REACTOR_STREAM_FLAG_ACTIVE;
  reactor_desc_clear(&stream->desc, REACTOR_DESC_MASK_READ);
  reactor_stream_flush(stream);
}

void reactor_stream_write(reactor_stream *stream, void *base, size_t size)
{
  buffer_insert(&stream->write, buffer_size(&stream->write), base, size);
}

void reactor_stream_write_string(reactor_stream *stream, char *string)
{
  reactor_stream_write(stream, string, strlen(string));
}

void reactor_stream_write_unsigned(reactor_stream *stream, uint32_t n)
{
  static const uint32_t pow10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  static const char digits[200] =
    "0001020304050607080910111213141516171819202122232425262728293031323334353637383940414243444546474849"
    "5051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
  uint32_t t, size, x;
  buffer *b;
  char *base;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  size = t - (n < pow10[t]) + 1;

  b = &stream->write;
  buffer_reserve(b, buffer_size(b) + size);
  b->size += size;
  base = (char *) buffer_data(b) + buffer_size(b);

  while (n >= 100)
    {
      x = (n % 100) << 1;
      n /= 100;
      *--base = digits[x + 1];
      *--base = digits[x];
    }
  if (n >= 10)
    {
      x = n << 1;
      *--base = digits[x + 1];
      *--base = digits[x];
    }
  else
    *--base = n + '0';
}

void reactor_stream_flush(reactor_stream *stream)
{
  ssize_t n;

  if (reactor_stream_blocked(stream))
    return;

  reactor_stream_hold(stream);
  if (buffer_size(&stream->write))
    {
      n = send(reactor_desc_fd(&stream->desc), buffer_data(&stream->write), buffer_size(&stream->write), MSG_DONTWAIT);
      if (n == -1 && errno != EAGAIN && reactor_stream_active(stream))
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_ERROR, stream);
      if (n > 0)
        buffer_erase(&stream->write, 0, n);
    }

  if (buffer_size(&stream->write))
    reactor_stream_set_blocked(stream);
  else if (stream->flags & REACTOR_STREAM_FLAG_LINGER)
    reactor_desc_close(&stream->desc);
  reactor_stream_release(stream);
}

void reactor_stream_set_blocked(reactor_stream *stream)
{
  reactor_desc_set(&stream->desc, REACTOR_DESC_MASK_WRITE);
  stream->flags |= REACTOR_STREAM_FLAG_BLOCKED;
}

void reactor_stream_clear_blocked(reactor_stream *stream)
{
  stream->flags &= ~REACTOR_STREAM_FLAG_BLOCKED;
  reactor_desc_clear(&stream->desc, REACTOR_DESC_MASK_WRITE);
}

int reactor_stream_blocked(reactor_stream *stream)
{
  return (stream->flags & REACTOR_STREAM_FLAG_BLOCKED) != 0;
}

void *reactor_stream_data_base(reactor_stream_data *data)
{
  return data->base;
}

size_t reactor_stream_data_size(reactor_stream_data *data)
{
  return data->size;
}

void reactor_stream_data_consume(reactor_stream_data *data, size_t size)
{
  data->base = (char *) data->base + size;
  data->size -= size;
}
