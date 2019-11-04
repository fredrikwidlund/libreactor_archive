#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static _Atomic int called = 0;
static _Atomic int returned = 0;

void event(void *state, int type, void *data)
{

  (void) state;
  (void) data;
  switch (type)
    {
    case REACTOR_POOL_EVENT_CALL:
      called ++;
      break;
    case REACTOR_POOL_EVENT_RETURN:
      returned ++;
      break;
    }
}

void core()
{
  int e, i;
  reactor_pool pool;

  reactor_core_construct();
  reactor_pool_construct(&pool);
  reactor_pool_enqueue(&pool, event, NULL);
  e = reactor_core_run();
  usleep(100000);
  reactor_pool_enqueue(&pool, event, NULL);
  e = reactor_core_run();
  assert_int_equal(called, returned);
  assert_int_equal(e, 0);
  reactor_pool_destruct(&pool);
  reactor_core_destruct();

  reactor_core_construct();
  reactor_pool_limits(&pool, 1, 16);
  reactor_pool_construct(&pool);
  for (i = 0; i < 10000; i ++)
    reactor_pool_enqueue(&pool, event, NULL);
  reactor_pool_destruct(&pool);
  reactor_core_destruct();
}

void scale()
{
  reactor_pool pool;
  int i;

  called = 0;
  returned = 0;
  reactor_core_construct();
  reactor_pool_construct(&pool);
  for (i = 0; i < 10000; i ++)
    reactor_pool_enqueue(&pool, event, NULL);
  reactor_core_run();
  assert_int_equal(called, returned);
  reactor_pool_destruct(&pool);
  reactor_core_destruct();
}

extern int mock_read_failure;
extern int mock_write_failure;

void io_error()
{
  reactor_pool pool;

  reactor_core_construct();
  reactor_pool_construct(&pool);
  reactor_pool_enqueue(&pool, event, NULL);
  usleep(100000);
  mock_read_failure = 1;
  reactor_core_run();
  mock_read_failure = 0;
  reactor_pool_destruct(&pool);
  reactor_core_destruct();

  reactor_core_construct();
  reactor_pool_construct(&pool);
  reactor_pool_enqueue(&pool, event, NULL);
  usleep(100000);
  close(pool.queue[0]);
  reactor_core_run();
  reactor_pool_destruct(&pool);
  reactor_core_destruct();

  reactor_core_construct();
  reactor_pool_construct(&pool);
  reactor_pool_enqueue(&pool, event, NULL);
  mock_write_failure = 1;
  usleep(100000);
  mock_write_failure = 0;
  reactor_pool_destruct(&pool);
  reactor_pool_destruct(&pool);
  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(scale),
    cmocka_unit_test(io_error)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}

