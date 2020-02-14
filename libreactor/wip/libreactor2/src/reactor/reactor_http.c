#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "dynamic.h"
#include "reactor.h"

char *reactor_http_headers_lookup(reactor_http_headers *headers, char *name)
{
  reactor_http_headers *h;
  size_t i;

  for (h = headers; h; h = h->next)
    for (i = 0; i < h->count; i ++)
      if (strcasecmp(h->header[i].name, name) == 0)
        return h->header[i].value;

  return NULL;
}

static void reactor_http_hold(reactor_http *http)
{
  http->refs ++;
}

static void reactor_http_release(reactor_http *http)
{
  http->refs --;
  if (!http->refs)
    reactor_user_dispatch(&http->user, REACTOR_TCP_DESTRUCT, http);
}

static void reactor_http_client_event(void *state, int type, void *data)
{
  reactor_http *http;
  reactor_http_response response;
  reactor_http_header header[REACTOR_HTTP_HEADERS_MAX];
  int token;

  http = state;
  switch (type)
    {
    case REACTOR_STREAM_READ:
      while (reactor_stream_data_size(data))
        {
          response.headers = (reactor_http_headers) {.count = REACTOR_HTTP_HEADERS_MAX, .header = header};
          token = reactor_http_parser_response(&http->parser, &response, data);
          if (reactor_likely(token == REACTOR_HTTP_PARSER_MESSAGE))
            reactor_user_dispatch(&http->user, REACTOR_HTTP_RESPONSE, &response);
          else
            {
              if (token == REACTOR_HTTP_PARSER_INVALID)
                {
                  reactor_stream_close(&http->stream);
                  reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, http);
                }
              break;
            }
        }
      break;
    case REACTOR_STREAM_ERROR:
      reactor_stream_close(&http->stream);
      reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, http);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_stream_close(&http->stream);
      reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, http);
      break;
    case REACTOR_STREAM_DESTRUCT:
      reactor_http_release(http);
    }
}

static void reactor_http_server_event(void *state, int type, void *data)
{
  reactor_http *http;
  reactor_http_request request;
  reactor_http_header header[REACTOR_HTTP_HEADERS_MAX];
  int token;

  http = state;
  reactor_http_hold(http);
  switch (type)
    {
    case REACTOR_STREAM_READ:
      http->flags |= REACTOR_HTTP_FLAG_FLUSH_LOCK;
      while (reactor_stream_data_size(data))
        {
          request.headers = (reactor_http_headers) {.count = REACTOR_HTTP_HEADERS_MAX, .header = header};
          token = reactor_http_parser_request(&http->parser, &request, data);
          if (reactor_likely(token == REACTOR_HTTP_PARSER_MESSAGE))
            reactor_user_dispatch(&http->user, REACTOR_HTTP_REQUEST, &request);
          else
            {
              if (token == REACTOR_HTTP_PARSER_INVALID)
                reactor_http_close(http);
              break;
            }
        }
      http->flags &= ~REACTOR_HTTP_FLAG_FLUSH_LOCK;
      reactor_http_flush(http);
      break;
    case REACTOR_STREAM_ERROR:
      reactor_stream_close(&http->stream);
      reactor_user_dispatch(&http->user, REACTOR_HTTP_ERROR, http);
      break;
    case REACTOR_STREAM_CLOSE:
      reactor_stream_close(&http->stream);
      reactor_user_dispatch(&http->user, REACTOR_HTTP_CLOSE, http);
      break;
    case REACTOR_STREAM_DESTRUCT:
      reactor_http_release(http);
    }
  reactor_http_release(http);
}

void reactor_http_construct(reactor_http *http, reactor_user_callback *callback, void *state, int fd, int flags)
{
  http->refs = 0;
  http->flags = REACTOR_HTTP_FLAG_ACTIVE;
  reactor_http_hold(http);
  reactor_user_construct(&http->user, callback, state);
  reactor_http_parser_construct(&http->parser);
  reactor_http_hold(http);
  reactor_stream_construct(&http->stream,
                           flags & REACTOR_HTTP_FLAG_SERVER ?
                           reactor_http_server_event :
                           reactor_http_client_event, http, fd);
}

void reactor_http_close(reactor_http *http)
{
  if (reactor_http_active(http))
    {
      http->flags &= ~REACTOR_HTTP_FLAG_ACTIVE;
      if (reactor_stream_active(&http->stream))
        reactor_stream_close(&http->stream);
      reactor_http_release(http);
    }
}

int reactor_http_active(reactor_http *http)
{
  return (http->flags & REACTOR_HTTP_FLAG_ACTIVE) != 0;
}

void reactor_http_send_request(reactor_http *http, reactor_http_request *request)
{
  reactor_stream *stream;
  reactor_http_headers *headers;
  size_t i;

  if (reactor_likely(reactor_http_active(http)))
    {
      stream = &http->stream;
      reactor_stream_write_string(stream, request->method);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, request->path);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, request->version ? "HTTP/1.1\r\n" : "HTTP/1.0\r\n");
      for (headers = &request->headers; headers; headers = headers->next)
        for (i = 0; i < headers->count; i ++)
          {
            reactor_stream_write_string(stream, headers->header[i].name);
            reactor_stream_write_string(stream, ": ");
            reactor_stream_write_string(stream, headers->header[i].value);
            reactor_stream_write_string(stream, "\r\n");
          }
      if (request->data)
        {
          reactor_stream_write_string(stream, "Content-Length: ");
          reactor_stream_write_unsigned(stream, request->size);
          reactor_stream_write_string(stream, "\r\n");
        }
      reactor_stream_write_string(stream, "\r\n");
      reactor_stream_write(stream, request->data, request->size);
    }
  reactor_http_flush(http);
}

void reactor_http_send_response(reactor_http *http, reactor_http_response *response)
{
  reactor_stream *stream;
  reactor_http_headers *headers;
  size_t i;

  if (reactor_likely(reactor_http_active(http)))
    {
      stream = &http->stream;
      reactor_stream_write_string(stream, response->version ? "HTTP/1.1 " : "HTTP/1.0 ");
      reactor_stream_write_unsigned(stream, response->status);
      reactor_stream_write_string(stream, " ");
      reactor_stream_write_string(stream, response->reason);
      reactor_stream_write_string(stream, "\r\n");
      for (headers = &response->headers; headers; headers = headers->next)
        for (i = 0; i < headers->count; i ++)
          {
            reactor_stream_write_string(stream, headers->header[i].name);
            reactor_stream_write_string(stream, ": ");
            reactor_stream_write_string(stream, headers->header[i].value);
            reactor_stream_write_string(stream, "\r\n");
          }
      reactor_stream_write_string(stream, "Content-Length: ");
      reactor_stream_write_unsigned(stream, response->size);
      reactor_stream_write_string(stream, "\r\n\r\n");
      if (response->data)
        reactor_stream_write(stream, response->data, response->size);
    }
  reactor_http_flush(http);
}

void reactor_http_flush(reactor_http *http)
{
  if (reactor_stream_active(&http->stream) && (http->flags & REACTOR_HTTP_FLAG_FLUSH_LOCK) == 0)
    reactor_stream_flush(&http->stream);
}
