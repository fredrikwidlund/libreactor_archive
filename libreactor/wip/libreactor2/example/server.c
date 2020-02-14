#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

struct server
{
  reactor_tcp    tcp;
  reactor_stream stream;
};

void stream_event(void *state, int type, void *arg)
{
  struct server *server;
  reactor_stream_data *data;

  server = state;
  switch (type)
    {
    case REACTOR_STREAM_READ:
      data = arg;
      printf("[stream read] %.*s\n", (int) data->size, (char *) data->base);
      data->size = 0;
      break;
    case REACTOR_STREAM_ERROR:
      printf("[stream error] %s\n", strerror(errno));
      break;
    case REACTOR_STREAM_CLOSE:
      printf("[stream close]\n");
      reactor_stream_close(&server->stream);
      break;
    case REACTOR_STREAM_DESTRUCT:
      printf("[stream destruct]\n");
      break;
    }
}

void tcp_event(void *state, int type, void *arg)
{
  struct server *server;

  (void) arg;
  server = state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      printf("[tcp accept]\n");
      reactor_tcp_close(&server->tcp);
      reactor_stream_construct(&server->stream, stream_event, server, *(int *) arg);
      break;
    case REACTOR_TCP_ERROR:
    case REACTOR_TCP_CLOSE:
      printf("[tcp failed]\n");
      reactor_tcp_close(&server->tcp);
      break;
    case REACTOR_TCP_DESTRUCT:
      printf("[tcp destruct]\n");
      break;
    }
}

int main(int argc, char **argv)
{
  struct server server;
  int e;

  if (argc != 3)
    errx(1, "usage: server <node> <service>\n");

  reactor_core_construct();
  reactor_tcp_construct(&server.tcp, tcp_event, &server, argv[1], argv[2], REACTOR_TCP_FLAG_SERVER);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
