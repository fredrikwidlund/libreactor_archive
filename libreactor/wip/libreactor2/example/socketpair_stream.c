#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

struct stream
{
  size_t         block_size;
  reactor_stream write;
  size_t         write_bytes;
  reactor_stream read;
  size_t         read_bytes;
};

void stream_event(void *state, int type, void *arg)
{
  struct stream *stream;
  reactor_stream_data *data;

  stream = state;
  switch(type)
    {
    case REACTOR_STREAM_READ:
      data = arg;
      stream->read_bytes -= data->size;
      data->size = 0;
      if (!stream->read_bytes)
        reactor_stream_close(&stream->read);
      break;
    case REACTOR_STREAM_WRITE:
      while (stream->write_bytes && !reactor_stream_blocked(&stream->write))
        {
          char block[stream->block_size];
          size_t n = MIN(stream->write_bytes, stream->block_size);
          reactor_stream_write(&stream->write, block, n);
          reactor_stream_flush(&stream->write);
          stream->write_bytes -= n;
        }
      if (!stream->write_bytes)
        reactor_stream_close(&stream->write);
      break;
    case REACTOR_STREAM_ERROR:
      break;
    case REACTOR_STREAM_DESTRUCT:
      break;
    }
}

int stream_init(struct stream *stream, size_t bytes, size_t block_size)
{
  int fd[2], e;

  *stream = (struct stream) {.block_size = block_size, .write_bytes = bytes, .read_bytes = bytes};
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  if (e == -1)
    return -1;
  reactor_stream_construct(&stream->write, stream_event, stream, fd[0]);
  reactor_stream_set_blocked(&stream->write);
  reactor_stream_construct(&stream->read, stream_event, stream, fd[1]);
  return 0;
}

int main(int argc, char **argv)
{
  struct stream stream;
  int e;

  if (argc != 3)
    errx(1, "usage: socketpair_stream <bytes> <block size>");
  reactor_core_construct();
  e = stream_init(&stream, strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0));
  if (e == -1)
    err(1, "stream_init");
  reactor_core_run();
  reactor_core_destruct();
}
