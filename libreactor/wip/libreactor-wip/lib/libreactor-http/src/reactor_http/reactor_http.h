#ifndef REACTOR_HTTP_SERVER_H_INCLUDED
#define REACTOR_HTTP_SERVER_H_INCLUDED

#ifndef REACTOR_HTTP_SERVER_REQUEST_MAX_HEADERS
#define REACTOR_HTTP_SERVER_REQUEST_MAX_HEADERS 128
#endif

enum REACTOR_HTTP_SERVER_EVENT
{
  REACTOR_HTTP_SERVER_ERROR = 0,
  REACTOR_HTTP_SERVER_REQUEST,
  REACTOR_HTTP_SERVER_CLOSE
};

typedef struct reactor_http_server reactor_http_server;
struct reactor_http_server
{
  reactor_tcp_server              tcp_server;
  reactor_user                    user;
};

typedef struct reactor_http_server_connection reactor_http_server_connection;
struct reactor_http_server_connection
{
  reactor_stream                  stream;
  uint32_t                        host;
  uint16_t                        port;
  size_t                          expect;
  reactor_http_server            *http_server;
};

typedef struct reactor_http_server_header reactor_http_server_header;
struct reactor_http_server_header
{
  const char                     *name;
  size_t                          name_len;
  const char                     *value;
  size_t                          value_len;
};

typedef struct reactor_http_server_request reactor_http_server_request;
struct reactor_http_server_request
{
  reactor_http_server_connection *connection;
  const char                     *method;
  size_t                          method_len;
  const char                     *path;
  size_t                          path_len;
  reactor_http_server_header     *headers;
  size_t                          num_headers;
  const char                     *body;
  size_t                          body_len;
};

void            reactor_http_server_init(reactor_http_server *, reactor_user_call *, void *);
int             reactor_http_server_open(reactor_http_server *, char *, char *);
void            reactor_http_server_event(void *, int, void *);
void            reactor_http_server_close(reactor_http_server *);

void            reactor_http_server_connection_init(reactor_http_server_connection *, reactor_http_server *, uint32_t, uint16_t);
int             reactor_http_server_connection_open(reactor_http_server_connection *, int);
void            reactor_http_server_connection_event(void *, int, void *);
void            reactor_http_server_connection_data(reactor_http_server_connection *, reactor_stream_buffer *);
int             reactor_http_server_connection_write(reactor_http_server_connection *, char *, size_t);
reactor_stream *reactor_http_server_connection_get_stream(reactor_http_server_connection *);
void            reactor_http_server_connection_shutdown(reactor_http_server_connection *);

#endif /* REACTOR_HTTP_SERVER_H_INCLUDED */
