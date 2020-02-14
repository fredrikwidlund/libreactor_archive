#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "dynamic.h"
#include "reactor.h"

static void reactor_http_server_map_release(void *data)
{
  reactor_http_server_map *map;

  map = data;
  free(map->method);
  free(map->path);
}

static void reactor_http_server_hold(reactor_http_server *server)
{
  server->refs ++;
}

static void reactor_http_server_release(reactor_http_server *server)
{
  server->refs --;
  if (!server->refs)
    {
      vector_destruct(&server->maps);
      if (server->user.callback)
        reactor_user_dispatch(&server->user, REACTOR_HTTP_SERVER_DESTRUCT, server);
    }
}

static void reactor_http_server_error(reactor_http_server *server)
{
  if (server->user.callback)
    reactor_user_dispatch(&server->user, REACTOR_HTTP_SERVER_ERROR, server);
}

static void reactor_http_server_date(reactor_http_server *server)
{
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t t;
  struct tm tm;

  (void) time(&t);
  (void) gmtime_r(&t, &tm);
  (void) strftime(server->date, sizeof server->date, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(server->date, days[tm.tm_wday], 3);
  memcpy(server->date + 8, months[tm.tm_mon], 3);
}

static reactor_http_server_map *reactor_http_server_lookup_map(reactor_http_server *server,
                                                               reactor_http_request *request)
{
  reactor_http_server_map *maps;
  size_t i;

  maps = vector_data(&server->maps);
  for (i = 0; i < vector_size(&server->maps); i ++)
    if ((!maps[i].method || strcmp(maps[i].method, request->method) == 0) &&
        (!maps[i].path || strcmp(maps[i].path, request->path) == 0))
      return &maps[i];

  return NULL;
}

static void reactor_http_server_tcp_event(void *state, int type, void *data)
{
  reactor_http_server *server;
  reactor_http_server_session *session;
  int *fd;

  server = state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      fd = data;
      session = malloc(sizeof *session);
      reactor_http_server_session_construct(session, server, *fd, REACTOR_HTTP_SERVER_SESSION_FLAG_FREE);
      break;
    case REACTOR_TCP_DESTRUCT:
      reactor_http_server_release(server);
      break;
    default:
      reactor_http_server_error(server);
      break;
    }
}

static void reactor_http_server_timer_event(void *state, int type, void *data)
{
  reactor_http_server *server;

  (void) data;
  server = state;
  switch (type)
    {
    case REACTOR_TIMER_CLOSE:
      reactor_timer_close(&server->timer);
      break;
    case REACTOR_TIMER_CALL:
      reactor_http_server_date(server);
      break;
    case REACTOR_TIMER_DESTRUCT:
      reactor_http_server_release(server);
      break;
    case REACTOR_TIMER_ERROR:
      reactor_http_server_error(server);
      break;
    }
}

void reactor_http_server_construct(reactor_http_server *server, reactor_user_callback *callback, void *state,
                                   char *host, char *port, int flags)
{
  server->refs = 0;
  server->flags = flags | REACTOR_HTTP_SERVER_FLAG_ACTIVE;
  server->name = "libreactor";
  reactor_http_server_date(server);
  vector_construct(&server->maps, sizeof(reactor_http_server_map));
  vector_release(&server->maps, reactor_http_server_map_release);
  reactor_user_construct(&server->user, callback, state);

  reactor_http_server_hold(server);
  reactor_tcp_construct(&server->tcp, reactor_http_server_tcp_event, server, host, port, REACTOR_HTTP_FLAG_SERVER);

  reactor_http_server_hold(server);
  reactor_timer_construct(&server->timer, reactor_http_server_timer_event, server, 1000000000, 1000000000);
}

void reactor_http_server_name(reactor_http_server *server, char *name)
{
  server->name = name;
}

int reactor_http_server_active(reactor_http_server *server)
{
  return (server->flags & REACTOR_HTTP_SERVER_FLAG_ACTIVE) != 0;
}

void reactor_http_server_close(reactor_http_server *server)
{
  if (reactor_http_server_active(server))
    {
      server->flags &= ~REACTOR_HTTP_SERVER_FLAG_ACTIVE;
      if (reactor_tcp_active(&server->tcp))
        reactor_tcp_close(&server->tcp);
      if (reactor_timer_active(&server->timer))
        reactor_timer_close(&server->timer);
    }
}

void reactor_http_server_match(reactor_http_server *server, reactor_user_callback *callback, void *state,
                               char *method, char *path)
{
  vector_push_back(&server->maps, (reactor_http_server_map[]) {{
        .type = REACTOR_HTTP_SERVER_MAP_MATCH,
        .user = {.callback = callback, .state = state},
        .method = method ? strdup(method) : NULL,
        .path = path ? strdup(path) : NULL}});
}

static void reactor_http_server_session_hold(reactor_http_server_session *session)
{
  session->refs ++;
}

static void reactor_http_server_session_release(reactor_http_server_session *session)
{
  session->refs --;
  if (!session->refs)
    {
      reactor_http_server_release(session->server);
      if (session->flags & REACTOR_HTTP_SERVER_SESSION_FLAG_FREE)
        free(session);
    }
}

static void reactor_http_server_session_event(void *state, int type, void *data)
{
  reactor_http_server_session *session;
  reactor_http_server_map *map;

  session = state;
  if (reactor_likely(type == REACTOR_HTTP_REQUEST))
    {
      reactor_http_server_session_hold(session);
      session->request = data;
      map = reactor_http_server_lookup_map(session->server, session->request);
      if (map)
        reactor_user_dispatch(&map->user, REACTOR_HTTP_SERVER_REQUEST, session);
      else
        reactor_http_server_session_respond_not_found(session);
    }
  else if (type == REACTOR_HTTP_DESTRUCT)
    reactor_http_server_session_release(session);
  else
    reactor_http_close(&session->http);
}

void reactor_http_server_session_construct(reactor_http_server_session *session, reactor_http_server *server, int fd, int flags)
{
  session->refs = 0;
  session->flags = flags | REACTOR_HTTP_SERVER_SESSION_FLAG_ACTIVE;
  session->server = server;
  session->request = NULL;
  reactor_http_server_hold(server);
  reactor_http_server_session_hold(session);
  reactor_http_construct(&session->http, reactor_http_server_session_event, session, fd, REACTOR_HTTP_FLAG_SERVER);
}

void reactor_http_server_session_respond(reactor_http_server_session *session, int status, char *reason,
                                 char *content_type, reactor_http_headers *headers, void *data, size_t size)
{
  reactor_http_send_response(&session->http, (reactor_http_response[]) {{
        .version = 1, .status = status, .reason = reason,
        .headers = {content_type ? 3 : 2, (reactor_http_header[]){
          {"Server", session->server->name},
          {"Date", session->server->date},
          {"Content-Type", content_type}
        }, headers}, .data = data, .size = size}});
  reactor_http_server_session_release(session);
}

void reactor_http_server_session_respond_text(reactor_http_server_session *session, char *text)
{
  reactor_http_server_session_respond(session, 200, "OK", "plain/text", NULL, text, strlen(text));
}

void reactor_http_server_session_respond_not_found(reactor_http_server_session *session)
{
  reactor_http_server_session_respond(session, 404, "Not Found", NULL, NULL, NULL, 0);
}
