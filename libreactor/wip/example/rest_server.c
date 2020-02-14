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
#include <err.h>
#include <dynamic.h>

#include <reactor_core.h>

static char reply[] =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Content-Length: 2\r\n"
  "\r\n"
  "OK";

void rest_event(void *state, int type, void *data)
{
  reactor_rest_request *request = data;

  if (type == REACTOR_REST_ERROR)
    warn("error event");

  if (type == REACTOR_REST_REQUEST)
    {
      reactor_stream_write(&request->session->stream, reply, strlen(reply));
      /*reactor_stream_printf(&request->session->stream,
			    "HTTP/1.1 %d %s\r\n"
			    "Content-Type: %s\r\n"
			    "Content-Length: %llu\r\n\r\nOK",
			    200, "OK", "text/html", 2);
      */
      //      if (size)
      //reactor_stream_write(&request->session->stream, body, size);

      //reactor_rest_return(request, 200, "OK", "text/html", "OK", 2);
      //reactor_stream_printf(&request->session->stream, "HTTP/1.1 200 OK\r\nContent-length: 2\r\nContent-type: text/html\r\n\r\nOK");
      //reactor_rest_return_text(request, request->state);
    }
}

int main()
{
  reactor_rest rest;

  reactor_core_construct();

  reactor_rest_init(&rest, rest_event, &rest);
  reactor_rest_open(&rest, NULL, NULL);
  reactor_rest_add(&rest, "GET", "/_status", "OK");

  reactor_core_run();

  reactor_core_destruct();
}
