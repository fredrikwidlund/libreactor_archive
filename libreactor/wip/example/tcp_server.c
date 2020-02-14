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
#include <netdb.h>
#include <dynamic.h>

#include <reactor_core.h>

void stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  reactor_stream_data *request;

  switch (type)
    {
    case REACTOR_STREAM_DATA:
      request = data;
      printf("[request] %.*s\n", (int) request->size, request->base);
      reactor_stream_printf(stream, "HTTP/1.0 200 OK\r\nContent-length: 2\r\n\r\nOK");
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_END:
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    default:
      break;
    }
}

void server_event(void *state, int type, void *data)
{
  reactor_tcp_server *server = state;
  reactor_stream *stream;
  reactor_tcp_server_data *client;
  char host[NI_MAXHOST], serv[NI_MAXSERV];

  (void) server;
  switch (type)
    {
    case REACTOR_TCP_SERVER_ACCEPT:
      client = data;
      getnameinfo((const struct sockaddr *__restrict) &client->addr, client->addr_len, host,
                  sizeof(host), serv, sizeof(server), NI_NUMERICHOST| NI_NUMERICSERV);
      printf("[client] %s:%s on %d\n", host, serv, client->fd);
      stream = malloc(sizeof *stream);
      reactor_stream_init(stream, stream_event, stream);
      reactor_stream_open(stream, client->fd);
      break;
    default:
      break;
    }
}

int main()
{
  reactor_tcp_server server;

  reactor_core_construct();
  reactor_tcp_server_init(&server, server_event, &server);
  reactor_tcp_server_open(&server, "localhost", "http");
  reactor_core_run();
  reactor_core_destruct();
}
