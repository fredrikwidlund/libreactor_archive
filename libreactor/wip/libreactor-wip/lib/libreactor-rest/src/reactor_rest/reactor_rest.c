#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <err.h>
#include <sys/socket.h>

#include <dynamic.h>
#include <reactor.h>
#include <reactor_http.h>

#include "reactor_rest/reactor_rest.h"

void reactor_rest_init(reactor_rest *server, reactor_user_call *call, void *state)
{
  reactor_http_server_init(&server->http_server, reactor_rest_event, server);
  reactor_user_init(&server->user, call, state);
  vector_init(&server->maps, sizeof(reactor_rest_map));
}

int reactor_rest_open(reactor_rest *server, char *node, char *service)
{
  return reactor_http_server_open(&server->http_server, node ? node : "0.0.0.0", service ? service : "http");
}

void reactor_rest_event(void *state, int type, void *data)
{
  reactor_rest *server;
  reactor_rest_map *map;
  reactor_rest_request request;
  size_t i;

  server = state;
  switch (type)
    {
    case REACTOR_HTTP_SERVER_REQUEST:        
      for (i = 0; i < vector_size(&server->maps); i ++)
	{
	  map = vector_at(&server->maps, i);
	  if (reactor_rest_map_match(map, data))
	    {
	      request = (reactor_rest_request) {.http_request = data, .state = map->state};
	      reactor_user_dispatch(&server->user, REACTOR_REST_REQUEST, &request);
	      return;
	    }
	}
      break;
    }
}

int reactor_rest_add(reactor_rest *server, char *method, char *path, void *state)
{
  reactor_rest_map map = {.method = strdup(method), .path = strdup(path), state = state};
  
  return vector_push_back(&server->maps, &map);
}

int reactor_rest_map_match(reactor_rest_map *map, reactor_http_server_request *request)
{
  return reactor_rest_match(map->method, request->method, request->method_len) &&
    reactor_rest_match(map->path, request->path, request->path_len);
}

int reactor_rest_match(char *s1, const char *s2, size_t s2_len)
{
  return strlen(s1) == s2_len && strncmp(s1, s2, s2_len) == 0;
}

void reactor_rest_response(reactor_rest_request *request, char *type, char *body, size_t size)
{
  reactor_stream *stream = &request->http_request->connection->stream;
  
  (void) reactor_stream_printf(stream, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %llu\r\n\r\n",
			       type, size, body);
  (void) reactor_stream_write(stream, body, size);
}

void reactor_rest_response_text(reactor_rest_request *request, char *text)
{
  reactor_rest_response(request, "text/plain", text, strlen(text));
}
