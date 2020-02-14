#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

#ifndef REACTOR_HTTP_HEADERS_MAX
#define REACTOR_HTTP_HEADERS_MAX 32
#endif

enum reactor_http_events
{
  REACTOR_HTTP_ERROR,
  REACTOR_HTTP_REQUEST,
  REACTOR_HTTP_RESPONSE,
  REACTOR_HTTP_CLOSE,
  REACTOR_HTTP_DESTRUCT
};

enum reactor_http_flags
{
  REACTOR_HTTP_FLAG_ACTIVE     = 0x01,
  REACTOR_HTTP_FLAG_SERVER     = 0x02,
  REACTOR_HTTP_FLAG_FLUSH_LOCK = 0x04
};

typedef struct reactor_http_header reactor_http_header;
struct reactor_http_header
{
  char                  *name;
  char                  *value;
};

typedef struct reactor_http_headers reactor_http_headers;
struct reactor_http_headers
{
  size_t                 count;
  reactor_http_header   *header;
  reactor_http_headers  *next;
};

typedef struct reactor_http_request reactor_http_request;
struct reactor_http_request
{
  int                    version;
  char                  *method;
  char                  *path;
  reactor_http_headers   headers;
  void                  *data;
  size_t                 size;
};

typedef struct reactor_http_response reactor_http_response;
struct reactor_http_response
{
  int                    version;
  int                    status;
  char                  *reason;
  reactor_http_headers   headers;
  void                  *data;
  size_t                 size;
};

typedef struct reactor_http reactor_http;
struct reactor_http
{
  int                    refs;
  int                    flags;
  reactor_user           user;
  reactor_http_parser    parser;
  reactor_stream         stream;
};

char *reactor_http_headers_lookup(reactor_http_headers *, char *);
void  reactor_http_construct(reactor_http *, reactor_user_callback *, void *, int , int);
void  reactor_http_close(reactor_http *);
int   reactor_http_active(reactor_http *);
void  reactor_http_flush(reactor_http *);
void  reactor_http_send_request(reactor_http *, reactor_http_request *);
void  reactor_http_send_response(reactor_http *, reactor_http_response *);

#endif /* REACTOR_HTTP_H_INCLUDED */
