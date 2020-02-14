#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <dynamic.h>
#include <reactor.h>

#include "picohttpparser/picohttpparser.h"
#include "reactor_http/reactor_http.h"

void reactor_http_server_init(reactor_http_server *http_server, reactor_user_call *call, void *state)
{
  (void) reactor_tcp_server_init(&http_server->tcp_server, reactor_http_server_event, http_server);
  (void) reactor_user_init(&http_server->user, call, state);
}

int reactor_http_server_open(reactor_http_server *http_server, char *node, char *service)
{
  return reactor_tcp_server_open(&http_server->tcp_server, node, service);
}

void reactor_http_server_event(void *state, int type, void *data)
{
  reactor_http_server *http_server;
  reactor_tcp_server_connection *tcp;
  reactor_http_server_connection *connection;
  char error[256];
  int e;

  http_server = state;
  switch (type)
    {
    case REACTOR_TCP_SERVER_ACCEPT:
      tcp = data;
      connection = malloc(sizeof *connection);
      reactor_log_debug("[http_server] new connection %p, fd %d\n", (void *) connection, tcp->fd);
      reactor_http_server_connection_init(connection, http_server, tcp->host, tcp->port);
      e = reactor_http_server_connection_open(connection, tcp->fd);
      if (e == -1)
	{
	  (void) close(tcp->fd);
	  free(connection);
	}
      break;
    case REACTOR_TCP_SERVER_CLOSE:
      reactor_user_dispatch(&http_server->user, REACTOR_HTTP_SERVER_CLOSE, NULL);
      break;
    case REACTOR_TCP_SERVER_ERROR:
      strerror_r(errno, error, sizeof error);
      reactor_log_debug("[http_server] tcp error %s\n", error);
      break;
    }
}

void reactor_http_server_close(reactor_http_server *http_server)
{
  reactor_tcp_server_close(&http_server->tcp_server);
}

void reactor_http_server_connection_init(reactor_http_server_connection *connection, reactor_http_server *http_server,
					 uint32_t host, uint16_t port)
{
  *connection = (reactor_http_server_connection) {.http_server = http_server, .host = host, .port = port};
  reactor_stream_init(&connection->stream, reactor_http_server_connection_event, connection);
}

int reactor_http_server_connection_open(reactor_http_server_connection *connection, int fd)
{
  return reactor_stream_open(&connection->stream, fd);
}

void reactor_http_server_connection_event(void *state, int type, void *data)
{
  reactor_http_server_connection *connection;
  
  connection = state;
  switch (type)
    {
    case REACTOR_STREAM_DATA:
      reactor_http_server_connection_data(connection, data);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_log_debug("[http_server] free connection %p\n", (void *) connection);
      free(connection);
      break;
    }
}

void reactor_http_server_connection_data(reactor_http_server_connection *connection, reactor_stream_buffer *buffer)
{
  reactor_http_server_request request;
  reactor_http_server_header headers[REACTOR_HTTP_SERVER_REQUEST_MAX_HEADERS];
  const char *method, *path;
  size_t method_len, path_len, body_len, num_headers, i;
  int minor_version, header_len;

  num_headers = REACTOR_HTTP_SERVER_REQUEST_MAX_HEADERS;
  header_len = phr_parse_request(buffer->base, buffer->size, &method, &method_len, &path, &path_len, &minor_version, (struct phr_header *)headers, &num_headers, 0);
  if (header_len == -1)
    {
      reactor_http_server_connection_shutdown(connection);
      return;
    }

  if (header_len == -2)
    {
      reactor_log_debug("[reactor_http_server_connection] partial header size %ld\n", buffer->size);
      return;
    }
  
  body_len = 0;
  for (i = 0; i < num_headers; i ++)
    if (strncasecmp(headers[i].name, "content-length", headers[i].name_len) == 0)
      body_len = strtoul(headers[i].value, NULL, 0);

  if (buffer->size < header_len + body_len)
    {
      connection->expect = header_len + body_len;
      reactor_log_debug("[reactor_http_server_connection] partial request size %ld, expect %ld\n", buffer->size, connection->expect);
      return;
    }

  request = (reactor_http_server_request) {.connection = connection, .method = method, .method_len = method_len,
					   .path = path, .path_len = path_len,
					   .headers = headers, .num_headers = num_headers,
					   .body = buffer->base + header_len, .body_len = body_len};
  reactor_user_dispatch(&connection->http_server->user, REACTOR_HTTP_SERVER_REQUEST, &request);

  connection->expect = 0;
  reactor_stream_buffer_consume(buffer, header_len + body_len);

  if (buffer->size)
    reactor_http_server_connection_data(connection, buffer);
}

int reactor_http_server_connection_write(reactor_http_server_connection *connection, char *data, size_t size)
{
  return reactor_stream_write(&connection->stream, data, size);
}

reactor_stream *reactor_http_server_connection_get_stream(reactor_http_server_connection *connection)
{
  return &connection->stream;
}

void reactor_http_server_connection_shutdown(reactor_http_server_connection *connection)
{
  reactor_stream_shutdown(&connection->stream);
}
