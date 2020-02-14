#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <err.h>
#include <sys/socket.h>
#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"
#include "reactor_tcp_server.h"
#include "reactor_http_server.h"
#include "reactor_rest.h"

void reactor_rest_init(reactor_rest *server, reactor_user_call *call, void *state)
{
  reactor_user_init(&server->user, call, state);
  reactor_http_server_init(&server->http_server, reactor_rest_event, server);
  vector_init(&server->maps, sizeof(reactor_rest_map));
}

int reactor_rest_open(reactor_rest *server, char *node, char *service)
{
  return reactor_http_server_open(&server->http_server, node, service);
}

void reactor_rest_event(void *state, int type, void *data)
{
  reactor_rest *server;
  reactor_rest_map *map;
  reactor_rest_request request;
  reactor_http_server_session *session;
  size_t i;

  server = state;
  switch (type)
    {
    case REACTOR_HTTP_SERVER_REQUEST:
      session = data;
      for (i = 0; i < vector_size(&server->maps); i ++)
        {
          map = vector_at(&server->maps, i);
          if (reactor_rest_match(map, &session->request))
            {
              request = (reactor_rest_request) {.session = data, .state = map->state};
              reactor_user_dispatch(&server->user, REACTOR_REST_REQUEST, &request);
              return;
            }
        }
      reactor_rest_return_not_found((reactor_rest_request[]){{.session = session}});
      break;
    case REACTOR_HTTP_SERVER_ERROR:
      reactor_user_dispatch(&server->user, REACTOR_REST_ERROR, data);
      break;
    }
}

int reactor_rest_add(reactor_rest *server, char *method, char *path, void *state)
{
  reactor_rest_map map = {.method = strdup(method), .path = strdup(path), state = state};

  return vector_push_back(&server->maps, &map);
}

int reactor_rest_match(reactor_rest_map *map, reactor_http_server_request *request)
{
  return ((!map->method || strcasecmp(map->method, request->method) == 0) &&
          (!map->path || strcasecmp(map->path, request->path) == 0));
}

void reactor_rest_return(reactor_rest_request *request, int code, char *reason, char *content_type, char *body, size_t size)
{
  reactor_stream_printf(&request->session->stream,
                        "HTTP/1.1 %d %s\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %llu\r\n\r\n",
                        code, reason, content_type, size, body);
  if (size)
    reactor_stream_write(&request->session->stream, body, size);
}

void reactor_rest_return_not_found(reactor_rest_request *request)
{
  char *not_found_body = "{\"status\":\"error\", \"code\":404, \"reason\":\"not found\"}";
  reactor_rest_return(request, 404, "Not Found", "application/json", not_found_body, strlen(not_found_body));
}

void reactor_rest_return_text(reactor_rest_request *request, char *body)
{
  reactor_rest_return(request, 200, "OK", "text/html", body, strlen(body));
}
