#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "dynamic.h"
#include "reactor.h"

void core(void **state)
{
  (void) state;
  reactor_core_construct();

  reactor_core_run();

  reactor_core_destruct();
}

void desc_event(void *state, int type, void *data)
{
  char buffer[256];
  ssize_t n;

  switch(type)
    {
    case REACTOR_DESC_READ:
      assert_true(state);
      assert_true(data);
      n = recv(reactor_desc_fd(state), buffer, sizeof buffer, 0);
      assert_int_equal(n, 5);
      assert_string_equal(buffer, "test");
      reactor_desc_clear(state, REACTOR_DESC_MASK_READ);
      reactor_desc_set(state, REACTOR_DESC_MASK_WRITE);
      break;
    case REACTOR_DESC_WRITE:
      reactor_desc_close(state);
      break;
    }
}

void desc(void **state)
{
  reactor_desc desc;
  int fd[2], e;

  (void) state;
  reactor_core_construct();

  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_desc_construct(&desc, desc_event, &desc, fd[0], REACTOR_DESC_MASK_READ);
  e = send(fd[1], "test", 5, 0);
  assert_int_equal(e, 5);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  e = close(fd[0]);
  assert_int_equal(e, -1);
  e = close(fd[1]);
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

#define BLOCKS 1024
static char block[1048576];
static int blocks = BLOCKS;
static int bytes_receive = BLOCKS * sizeof(block);

void stream_event(void *state, int type, void *arg)
{
  reactor_stream *stream;
  reactor_stream_data *data;

  stream = state;
  switch(type)
    {
    case REACTOR_STREAM_READ:
      data = arg;
      bytes_receive -= data->size;
      data->size = 0;
      if (!bytes_receive)
        reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_WRITE:
      if (!blocks)
        reactor_stream_close(stream);
      while (blocks && !reactor_stream_blocked(stream))
        {
          reactor_stream_write(stream, block, sizeof block);
          reactor_stream_flush(stream);
          blocks --;
        }
      break;
    case REACTOR_STREAM_ERROR:
      assert_true(0);
      break;
    }
}

void stream(void **state)
{
  reactor_stream out, in;
  int fd[2], e;

  (void) state;
  reactor_core_construct();

  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  fcntl(fd[0], F_SETFL, O_NONBLOCK);
  reactor_stream_construct(&out, stream_event, &out, fd[0]);
  reactor_stream_construct(&in, stream_event, &in, fd[1]);
  reactor_stream_set_blocked(&out);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(desc),
    cmocka_unit_test(stream)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);

  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
