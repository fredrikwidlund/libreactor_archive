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
#include <sys/socket.h>
#include <netdb.h>
#include <dynamic.h>

#include <reactor_core.h>

static char reply[] =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Content-Length: 2\r\n"
  "\r\n"
  "OK";

void server_event(void *state, int type, void *data)
{
  reactor_http_server_session *session = data;

  if (type == REACTOR_HTTP_SERVER_REQUEST)
    {
      reactor_stream_write(&session->stream, reply, strlen(reply));
      //reactor_stream_printf(&session->stream, "HTTP/1.1 200 OK\r\nContent-length: 2\r\nContent-type: text/html\r\n\r\nOK");
    }
}

int main()
{
  reactor_http_server server;

  reactor_core_construct();
  reactor_http_server_init(&server, server_event, &server);
  reactor_http_server_open(&server, "localhost", "http");
  reactor_core_run();
  reactor_core_destruct();
}
