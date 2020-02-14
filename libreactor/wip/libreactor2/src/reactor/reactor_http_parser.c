#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <dynamic.h>
#include <reactor.h>

#include "picohttpparser/picohttpparser.h"

static int reactor_http_parser_invalid(reactor_http_parser *parser, reactor_stream_data *data)
{
  reactor_stream_data_consume(data, reactor_stream_data_size(data));
  parser->state = REACTOR_HTTP_PARSER_STATE_INVALID;
  return REACTOR_HTTP_PARSER_INVALID;
}

void reactor_http_parser_construct(reactor_http_parser *parser)
{
  *parser = (reactor_http_parser) {.state = REACTOR_HTTP_PARSER_STATE_HEADER};
}

int reactor_http_parser_request(reactor_http_parser *parser, reactor_http_request *request, reactor_stream_data *data)
{
  struct phr_header phr_header[request->headers.count];
  size_t method_size, path_size, i;
  off_t offset;
  int n;

  if (reactor_unlikely(parser->state == REACTOR_HTTP_PARSER_STATE_INVALID))
    return reactor_http_parser_invalid(parser, data);

  if (reactor_likely(parser->flags == REACTOR_HTTP_PARSER_STATE_HEADER))
    {
      n = phr_parse_request(reactor_stream_data_base(data), reactor_stream_data_size(data),
                            (const char **) &request->method, &method_size,
                            (const char **) &request->path, &path_size,
                            &request->version, phr_header, &request->headers.count, 0);
      if (reactor_unlikely(n == -1))
        return reactor_http_parser_invalid(parser, data);

      if (reactor_unlikely(n <= 0))
        return REACTOR_HTTP_PARSER_INCOMPLETE;

      parser->header_base = reactor_stream_data_base(data);
      parser->header_size = n;
      request->method[method_size] = 0;
      request->path[path_size] = 0;
      request->size = 0;
      for (i = 0; i < request->headers.count; i ++)
        {
          request->headers.header[i].name = (char *) phr_header[i].name;
          request->headers.header[i].name[phr_header[i].name_len] = 0;
          request->headers.header[i].value = (char *) phr_header[i].value;
          request->headers.header[i].value[phr_header[i].value_len] = 0;
          if (strcasecmp(request->headers.header[i].name, "content-length") == 0)
            {
              parser->flags |= REACTOR_HTTP_PARSER_CONTENT_LENGTH;
              parser->body_size = strtoul(request->headers.header[i].value, NULL, 10);
            }
          else if ((strcasecmp(request->headers.header[i].name, "transfer-encoding") == 0 &&
                    strcasecmp(request->headers.header[i].name, "chunked") == 0))
            parser->flags |= REACTOR_HTTP_PARSER_CHUNKED;
        }
      request->headers.next = NULL;

      if (reactor_unlikely(parser->flags & REACTOR_HTTP_PARSER_CHUNKED))
        return reactor_http_parser_invalid(parser, data);

      parser->state = REACTOR_HTTP_PARSER_STATE_BODY;
    }

  if (reactor_unlikely(reactor_stream_data_size(data) < parser->header_size + parser->body_size))
    return REACTOR_HTTP_PARSER_INCOMPLETE;

  if (reactor_unlikely(reactor_stream_data_base(data) != parser->header_base))
    {
      offset = (char *) reactor_stream_data_base(data) - (char *) parser->header_base;
      request->method += offset;
      request->path += offset;
      for (i = 0; i < request->headers.count; i ++)
        {
          request->headers.header[i].name += offset;
          request->headers.header[i].value += offset;
        }
    }

  request->data = (char *) reactor_stream_data_base(data) + parser->header_size;
  request->size = parser->body_size;
  reactor_stream_data_consume(data, parser->header_size + parser->body_size);
  reactor_http_parser_construct(parser);
  return REACTOR_HTTP_PARSER_MESSAGE;
}

int reactor_http_parser_response(reactor_http_parser *parser, reactor_http_response *response, reactor_stream_data *data)
{
  struct phr_header phr_header[response->headers.count];
  size_t reason_size, i;
  off_t offset;
  int n;

  if (reactor_unlikely(parser->state == REACTOR_HTTP_PARSER_STATE_INVALID))
    return reactor_http_parser_invalid(parser, data);

  if (reactor_likely(parser->flags == REACTOR_HTTP_PARSER_STATE_HEADER))
    {
      n = phr_parse_response(reactor_stream_data_base(data), reactor_stream_data_size(data),
                             &response->version,
                             &response->status,
                             (const char **) &response->reason, &reason_size,
                             phr_header, &response->headers.count, 0);
      if (reactor_unlikely(n == -1))
        return reactor_http_parser_invalid(parser, data);

      if (reactor_unlikely(n <= 0))
        return REACTOR_HTTP_PARSER_INCOMPLETE;

      parser->header_base = reactor_stream_data_base(data);
      parser->header_size = n;
      response->reason[reason_size] = 0;
      for (i = 0; i < response->headers.count; i ++)
        {
          response->headers.header[i].name = (char *) phr_header[i].name;
          response->headers.header[i].name[phr_header[i].name_len] = 0;
          response->headers.header[i].value = (char *) phr_header[i].value;
          response->headers.header[i].value[phr_header[i].value_len] = 0;
          if (strcasecmp(response->headers.header[i].name, "content-length") == 0)
            {
              parser->flags |= REACTOR_HTTP_PARSER_CONTENT_LENGTH;
              parser->body_size = strtoul(response->headers.header[i].value, NULL, 10);
            }
          else if (strcasecmp(response->headers.header[i].name, "transfer-encoding") == 0 &&
                   strcasecmp(response->headers.header[i].name, "chunked") == 0)
            parser->flags |= REACTOR_HTTP_PARSER_CHUNKED;
        }
      response->headers.next = NULL;

      if ((parser->flags & REACTOR_HTTP_PARSER_CONTENT_LENGTH) == 0)
        return reactor_http_parser_invalid(parser, data);

      parser->state = REACTOR_HTTP_PARSER_STATE_BODY;
    }

  if (reactor_unlikely(reactor_stream_data_size(data) < parser->header_size + parser->body_size))
    return REACTOR_HTTP_PARSER_INCOMPLETE;

  if (reactor_unlikely(reactor_stream_data_base(data) != parser->header_base))
    {
      offset = (char *) reactor_stream_data_base(data) - (char *) parser->header_base;
      response->reason += offset;
      for (i = 0; i < response->headers.count; i ++)
        {
          response->headers.header[i].name +=  offset;
          response->headers.header[i].value += offset;
        }
    }

  response->data = (char *) reactor_stream_data_base(data) + parser->header_size;
  response->size = parser->body_size;
  reactor_stream_data_consume(data, parser->header_size + parser->body_size);
  reactor_http_parser_construct(parser);
  return REACTOR_HTTP_PARSER_MESSAGE;
}
