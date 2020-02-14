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

typedef struct client client;
struct client
{
  reactor_resolver resolver;
  reactor_stream   stream;
};

void stream_event(void *state, int type, void *data)
{
  client *client;

  client = state;
  printf("stream event %d\n", type);
  if (type == REACTOR_STREAM_CLOSE)
    return;

  reactor_stream_printf(&client->stream, "HI");
  reactor_stream_flush(&client->stream);
  reactor_stream_close(&client->stream);
}

void resolver_event(void *state, int type, void *data)
{
  client *client = state;
  struct addrinfo *ai = data;
  int e;

  if (type == REACTOR_RESOLVER_ERROR)
    err(1, "resolver error");

  if (!ai)
    errx(1, "host not found");

  e = reactor_tcp_connect(&client->stream, ai);
  freeaddrinfo(ai);
  if (e == -1)
    err(1, "connect error");
}

int main(int argc, char **argv)
{
  client client;

  reactor_core_construct();

  reactor_stream_init(&client.stream, stream_event, &client);
  reactor_resolver_init(&client.resolver, resolver_event, &client);
  reactor_resolver_lookup(&client.resolver, argv[1], "http", NULL);
  reactor_core_run();
  reactor_core_destruct();

  wait(NULL);
}
