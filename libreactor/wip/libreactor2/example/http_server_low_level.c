#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "dynamic.h"
#include "reactor.h"

void server_http_event(void *state, int type, void *data)
{
  reactor_http *http;

  http = state;
  (void) data;
  if (reactor_likely(type == REACTOR_HTTP_REQUEST))
    {
      reactor_http_send_response(http, (reactor_http_response[]) {{
            .version = 1, .status = 200, .reason = "OK",
            .headers = {3, (reactor_http_header[]){
                {"Server", "*"}, {"Date", "Tue, 29 Nov 2016 22:17:05 GMT"}, {"Content-Type", "text/plain"}
              }, NULL}, .data = "Hello, World!", .size = 13}});
      reactor_http_close(http);
    }
  else if (type == REACTOR_HTTP_DESTRUCT)
    free(http);
}

void server_tcp_event(void *state, int type, void *data)
{
  reactor_http *http;

  (void) state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      http = malloc(sizeof *http);
      reactor_http_construct(http, server_http_event, http, *(int *) data, REACTOR_HTTP_FLAG_SERVER);
      break;
    default:
      err(1, "tcp error\n");
    }
}

int main()
{
  reactor_tcp tcp;

  reactor_core_construct();
  reactor_tcp_construct(&tcp, server_tcp_event, &tcp, "127.0.0.1", "80", REACTOR_TCP_FLAG_SERVER);
  (void) reactor_core_run();
  reactor_core_destruct();
}
