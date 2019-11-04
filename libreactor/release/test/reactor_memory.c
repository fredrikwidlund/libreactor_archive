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

void core()
{
  reactor_memory memory;

  memory = reactor_memory_str("test");
  assert_false(reactor_memory_empty(memory));
  assert_true(reactor_memory_equal(memory, reactor_memory_ref("test", 4)));
  assert_false(reactor_memory_equal(memory, reactor_memory_ref("testx", 5)));
  assert_false(reactor_memory_equal(memory, reactor_memory_ref("tesx", 4)));
  assert_true(reactor_memory_equal_case(memory, reactor_memory_ref("tEsT", 4)));
  assert_false(reactor_memory_equal_case(memory, reactor_memory_ref("tEsTx", 5)));
  assert_false(reactor_memory_equal_case(memory, reactor_memory_ref("tEsx", 4)));
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
