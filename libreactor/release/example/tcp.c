#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

typedef struct connection connection;
struct connection
{
  reactor_stream  stream;
  reactor_tcp    *tcp;
};

void stream_event(void *state, int type, void *data)
{
  connection *c = state;
  reactor_memory *read = data;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      (void) printf("[read] %.*s\n", (int) reactor_memory_size(*read), reactor_memory_base(*read));
      if (memmem(read->base, read->size, "quit", 4))
        reactor_tcp_close(c->tcp);
      reactor_stream_consume(&c->stream, reactor_memory_size(*read));
      break;
    case REACTOR_STREAM_EVENT_ERROR:
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_stream_close(&c->stream);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      (void) printf("[close]\n");
      free(c);
      break;
    }
}

void tcp_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  connection *c;
  int s;

  switch (type)
    {
    case REACTOR_TCP_EVENT_ERROR:
      (void) fprintf(stderr, "error\n");
      reactor_tcp_close(tcp);
      break;
    case REACTOR_TCP_EVENT_ACCEPT:
    case REACTOR_TCP_EVENT_CONNECT:
      s = *(int *) data;
      (void) printf("[%d]\n", s);
      c = malloc(sizeof *c);
      c->tcp = tcp;
      reactor_stream_open(&c->stream, stream_event, c, s);
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_tcp tcp;
  int e;

  if (argc != 4 ||
      (strcmp(argv[1], "client") == 0 &&
       strcmp(argv[1], "server") == 0))
    err(1, "usage: tcp [client|server] <host> <port>");

  reactor_core_construct();
  reactor_tcp_open(&tcp, tcp_event, &tcp, argv[2], argv[3],
                   strcmp(argv[1], "server") == 0 ? REACTOR_TCP_FLAG_SERVER : 0);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
