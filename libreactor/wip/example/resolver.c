#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <dynamic.h>
#include <reactor_core.h>

void event(void *state, int type, void *data)
{
  reactor_resolver *resolver = state;

  if (type == REACTOR_RESOLVER_REPLY)
    {
      printf("reply %p\n", data);
      free(data);
    }
}

int main(int argc, char **argv)
{
  reactor_resolver resolver;
  reactor_tcp_client client;

  reactor_core_construct();

  reactor_resolver_init(&resolver, event, &resolver);
  reactor_resolver_lookup(&resolver, argv[1], "http", NULL);
  reactor_core_run();
  reactor_core_destruct();
}
