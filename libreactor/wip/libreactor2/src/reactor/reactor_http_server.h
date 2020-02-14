#ifndef REACTOR_HTTP_SERVER_H_INCLUDED
#define REACTOR_HTTP_SERVER_H_INCLUDED

enum reactor_http_server_events
{
  REACTOR_HTTP_SERVER_ERROR,
  REACTOR_HTTP_SERVER_REQUEST,
  REACTOR_HTTP_SERVER_CLOSE,
  REACTOR_HTTP_SERVER_DESTRUCT
};

enum reactor_http_server_flags
{
  REACTOR_HTTP_SERVER_FLAG_ACTIVE = 0x01,
};

enum reactor_http_server_map_types
{
  REACTOR_HTTP_SERVER_MAP_MATCH
};

typedef struct reactor_http_server_map reactor_http_server_map;
struct reactor_http_server_map
{
  int                   type;
  reactor_user          user;
  char                 *method;
  char                 *path;
};

typedef struct reactor_http_server reactor_http_server;
struct reactor_http_server
{
  int                   refs;
  int                   flags;
  reactor_user          user;
  reactor_tcp           tcp;
  reactor_timer         timer;
  vector                maps;
  char                 *name;
  char                  date[32];
};

enum reactor_http_server_session_flags
{
  REACTOR_HTTP_SERVER_SESSION_FLAG_ACTIVE = 0x01,
  REACTOR_HTTP_SERVER_SESSION_FLAG_FREE   = 0x02
};

typedef struct reactor_http_server_session reactor_http_server_session;
struct reactor_http_server_session
{
  int                   refs;
  int                   flags;
  reactor_http          http;
  reactor_http_server  *server;
  reactor_http_request *request;
};

void reactor_http_server_construct(reactor_http_server *, reactor_user_callback *, void *, char *, char *, int);
void reactor_http_server_name(reactor_http_server *, char *);
int  reactor_http_server_active(reactor_http_server *);
void reactor_http_server_close(reactor_http_server *);
void reactor_http_server_match(reactor_http_server *, reactor_user_callback *, void *, char *, char *);

void reactor_http_server_session_construct(reactor_http_server_session *, reactor_http_server *, int, int);
void reactor_http_server_session_respond(reactor_http_server_session *, int, char *, char *, reactor_http_headers *, void *, size_t);
void reactor_http_server_session_respond_text(reactor_http_server_session *, char *);
void reactor_http_server_session_respond_not_found(reactor_http_server_session *);

#endif /* REACTOR_HTTP_SERVER_H_INCLUDED */
