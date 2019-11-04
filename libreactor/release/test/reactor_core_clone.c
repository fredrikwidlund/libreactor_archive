#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

void job_event(void *state, int type, void *data)
{
  assert_true(state == NULL);
  assert_true(data == NULL);
  assert_true(type == REACTOR_POOL_EVENT_CALL ||
              type == REACTOR_POOL_EVENT_RETURN);
}

void job()
{
  int e;
  reactor_core_construct();
  reactor_core_job_register(job_event, NULL);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(job),
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
