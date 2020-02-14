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

typedef struct transaction transaction;
struct transaction
{
  reactor_http_client   client;
  reactor_http_session *session;
};

void client_event(void *state, int type, void *data)
{
  transaction *t;
  reactor_http_response *response;
  reactor_http_response_context *context;

  t = state;
  switch (type)
    {
    case REACTOR_HTTP_CLIENT_RESPONSE:
      context = data;
      response = context->response;
      reactor_http_session_close(context->session);

      fprintf(stderr, "[http_proxy] upstream status %d, size %lu\n", response->status, response->size);
      fprintf(stderr, "[http_proxy] client session %p\n", (void *) t->session);
      reactor_http_server_respond(t->session, response->status, response->reason,
                                  reactor_http_headers_lookup(&response->headers, "content-type"),
                                  NULL, response->data, response->size);
      reactor_http_session_close(t->session);
      // Will break if pipeline of 2 reqs are sent since second will write to freed session
      reactor_http_client_close(&t->client);
      //reactor_stream_close(&t->request.session->stream);
      //reactor_stream_close(&response->session->stream);
      //reactor_http_client_close(&t->client);
      // reactor_http_session_reply_body(...)
      // reactor_http_session_flush(...)
      // reactor_http_session_close(...)
      break;
    case REACTOR_HTTP_CLIENT_ERROR:
      fprintf(stderr, "[http_proxy] upstream error\n");
      // reactor_http_session_close(...)
      //reactor_stream_close(&tx->session->stream);

      //reactor_http_client_close(&tx->client);
      break;
    case REACTOR_HTTP_CLIENT_DESTRUCT:
      printf("client destruct\n");
      free(t);
      break;
    }
}

void debug_event(void *state, int type, void *data)
{
  printf("%p %d %p\n", state, type, data);
}

void proxy_event(void *state, int type, void *data)
{
  reactor_http_request_context *context;
  transaction *t;

  switch (type)
    {
    case REACTOR_HTTP_SERVER_REQUEST:
      context = data;
      t = malloc(sizeof *t);
      t->session = context->session;
      reactor_http_client_construct_get(&t->client, client_event, t, "127.0.0.1", "80", "/");
      break;
    default:
      printf("[proxy] state %p, type %d\n", state, type);
      break;
    }
}

void upstream_event(void *state, int type, void *data)
{
  reactor_http_request_context *context;

  switch (type)
    {
    case REACTOR_HTTP_SERVER_REQUEST:
      context = data;
      reactor_http_server_respond_text(context->session, "Hello, World!");
      break;
    default:
      printf("[proxy] state %p, type %d\n", state, type);
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_http_server upstream;
  reactor_http_server proxy;
  int e;

  (void) argv;
  if (argc != 1)
    errx(1, "usage: http_proxy\n");

  signal(SIGPIPE, SIG_IGN);

  reactor_core_construct();
  reactor_http_server_construct(&proxy, debug_event, &proxy, "127.0.0.1", "81", 0);
  reactor_http_server_match(&proxy, proxy_event, &proxy, "GET", "/");
  reactor_http_server_construct(&upstream, debug_event, &upstream, "127.0.0.1", "80", 0);
  reactor_http_server_match(&upstream, upstream_event, &upstream, "GET", "/");
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
