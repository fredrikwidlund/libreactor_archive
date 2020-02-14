#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <dynamic.h>

#include "reactor_core.h"

extern int debug_io_error;

static int event_called = 0;

void event(void *state, int type, void *data)
{
  (void) state;
  (void) type;
  (void) data;

  printf("type %d\n", type); 
  event_called ++;
}

void coverage()
{
  reactor_tcp_server server;
  int e;

  reactor_core_construct();
  reactor_tcp_server_init(&server, event, &server);

  e = reactor_tcp_server_open(&server, "localhost", "8080");
  assert_int_equal(e, 0);

  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(event_called >= 5);

  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
