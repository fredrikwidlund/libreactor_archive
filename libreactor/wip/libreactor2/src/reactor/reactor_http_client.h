#ifndef REACTOR_HTTP_CLIENT_H_INCLUDED
#define REACTOR_HTTP_CLIENT_H_INCLUDED

enum reactor_http_client_events
{
  REACTOR_HTTP_CLIENT_ERROR,
  REACTOR_HTTP_CLIENT_RESPONSE,
  REACTOR_HTTP_CLIENT_DESTRUCT
};

enum reactor_http_client_flags
{
  REACTOR_HTTP_CLIENT_FLAG_ACTIVE = 0x01,
};

typedef struct reactor_http_client reactor_http_client;
struct reactor_http_client
{
  int                   refs;
  int                   flags;
  reactor_user          user;
  reactor_tcp           tcp;
  reactor_http          http;
  char                 *host;
  char                 *port;
  char                 *method;
  char                 *path;
  reactor_http_headers *headers;
  void                 *data;
  size_t                size;
};

void reactor_http_client_construct(reactor_http_client *, reactor_user_callback *, void *,
                                   char *, char *, char *, char *,
                                   reactor_http_headers *, void *, size_t, int);
void reactor_http_client_construct_get(reactor_http_client *, reactor_user_callback *, void *,
                                       char *, char *, char *);
int  reactor_http_client_active(reactor_http_client *);
void reactor_http_client_close(reactor_http_client *);

#endif /* REACTOR_HTTP_CLIENT_H_INCLUDED */
