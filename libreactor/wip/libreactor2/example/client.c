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

struct client
{
  reactor_tcp    tcp;
  reactor_stream stream;
};

void stream_event(void *state, int type, void *arg)
{
  struct client *client;
  reactor_stream_data *data;

  client = state;
  switch (type)
    {
    case REACTOR_STREAM_READ:
      data = arg;
      printf("[stream read] %.*s\n", (int) data->size, (char *) data->base);
      data->size = 0;
      break;
    case REACTOR_STREAM_ERROR:
    case REACTOR_STREAM_CLOSE:
      printf("[stream close]\n");
      reactor_stream_close(&client->stream);
      break;
    case REACTOR_STREAM_DESTRUCT:
      printf("[stream destruct]\n");
      break;
    }
}

void tcp_event(void *state, int type, void *arg)
{
  struct client *client;

  client = state;
  switch (type)
    {
    case REACTOR_TCP_CONNECT:
      printf("[connect succeeded]\n");
      reactor_tcp_close(&client->tcp);
      reactor_stream_construct(&client->stream, stream_event, client, *(int *) arg);
      break;
    case REACTOR_TCP_ERROR:
    case REACTOR_TCP_CLOSE:
      printf("[connect failed]\n");
      reactor_tcp_close(&client->tcp);
      break;
    case REACTOR_TCP_DESTRUCT:
      printf("[tcp destruct]\n");
      break;
    }
}

int main(int argc, char **argv)
{
  struct client client;
  int e;

  if (argc != 3)
    errx(1, "usage: client <node> <service>\n");

  reactor_core_construct();
  reactor_tcp_construct(&client.tcp, tcp_event, &client, argv[1], argv[2], 0);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
