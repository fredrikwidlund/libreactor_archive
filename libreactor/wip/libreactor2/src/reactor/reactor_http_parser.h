#ifndef REACTOR_HTTP_PARSER_H_INCLUDED
#define REACTOR_HTTP_PARSER_H_INCLUDED

enum reactor_http_parser_token
{
  REACTOR_HTTP_PARSER_INVALID,
  REACTOR_HTTP_PARSER_INCOMPLETE,
  REACTOR_HTTP_PARSER_HEADER,
  REACTOR_HTTP_PARSER_BODY,
  REACTOR_HTTP_PARSER_MESSAGE
};

enum reactor_http_parser_state
{
  REACTOR_HTTP_PARSER_STATE_HEADER = 0,
  REACTOR_HTTP_PARSER_STATE_BODY,
  REACTOR_HTTP_PARSER_STATE_INVALID
};

enum reactor_http_parser_flags
{
  REACTOR_HTTP_PARSER_CONTENT_LENGTH = 0x01,
  REACTOR_HTTP_PARSER_CHUNKED        = 0x02,
};

typedef struct reactor_http_parser reactor_http_parser;
struct reactor_http_parser
{
  int     state;
  int     flags;
  void   *header_base;
  size_t  header_size;
  size_t  body_size;
};

/* reactor_http_request/reactor_http_response are declared later */
typedef struct reactor_http_request reactor_http_request;
typedef struct reactor_http_response reactor_http_response;

void reactor_http_parser_construct(reactor_http_parser *);
int  reactor_http_parser_request(reactor_http_parser *, reactor_http_request *, reactor_stream_data *);
int  reactor_http_parser_response(reactor_http_parser *, reactor_http_response *, reactor_stream_data *);

#endif /* REACTOR_HTTP_PARSER_H_INCLUDED */

