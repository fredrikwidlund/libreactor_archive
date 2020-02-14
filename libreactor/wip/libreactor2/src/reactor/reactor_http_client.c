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

static void reactor_http_client_hold(reactor_http_client *client)
{
  client->refs ++;
}

static void reactor_http_client_release(reactor_http_client *client)
{
  client->refs --;
  if (!client->refs)
    {
      free(client->host);
      reactor_user_dispatch(&client->user, REACTOR_HTTP_CLIENT_DESTRUCT, client);
    }
}

static void reactor_http_client_error(reactor_http_client *client)
{
  reactor_user_dispatch(&client->user, REACTOR_HTTP_CLIENT_ERROR, client);
}

static void reactor_http_client_http_event(void *state, int type, void *data)
{
  reactor_http_client *client;
  reactor_http_response *response;

  client = state;
  reactor_http_client_hold(client);
  switch (type)
    {
    case REACTOR_HTTP_RESPONSE:
      response = data;
      reactor_http_close(&client->http);
      reactor_user_dispatch(&client->user, REACTOR_HTTP_CLIENT_RESPONSE, response);
      break;
    case REACTOR_HTTP_CLOSE:
    case REACTOR_HTTP_ERROR:
      reactor_http_client_error(client);
      break;
    case REACTOR_HTTP_DESTRUCT:
      reactor_http_client_release(client);
      break;
    }
  reactor_http_client_release(client);
}

void reactor_http_client_tcp_event(void *state, int type, void *data)
{
  reactor_http_client *client = state;
  char host[strlen(client->host) + strlen(client->port) + 2];

  switch (type)
    {
    case REACTOR_TCP_CONNECT:
      strcpy(host, client->host);
      if (strcmp(client->port, "80") != 0 && strcmp(client->port, "http") != 0)
        {
          strcat(host, ":");
          strcat(host, client->port);
        }
      reactor_tcp_close(&client->tcp);
      reactor_http_client_hold(client);
      reactor_http_construct(&client->http, reactor_http_client_http_event, client, *(int *) data, 0);
      reactor_http_send_request(&client->http, (reactor_http_request[]) {{
            .version = 1,
            .method = "GET",
            .path = client->path,
              .headers = {2, (reactor_http_header[]){{"Host", host}, {"Connection", "close"}}, NULL},
            .data = NULL,
            .size = 0
            }});
      reactor_http_flush(&client->http);
      break;
    case REACTOR_TCP_CLOSE:
    case REACTOR_TCP_ERROR:
      reactor_http_client_error(client);
      break;
    case REACTOR_TCP_DESTRUCT:
      reactor_http_client_release(client);
      break;
    }
}

void reactor_http_client_construct(reactor_http_client *client, reactor_user_callback *callback, void *state,
                                   char *method, char *host, char *port, char *path,
                                   reactor_http_headers *headers, void *data, size_t size, int flags)
{
  client->refs = 0;
  client->flags = flags | REACTOR_HTTP_CLIENT_FLAG_ACTIVE;
  reactor_http_client_hold(client);
  client->host = host;
  client->port = port;
  client->method = method;
  client->path = path;
  client->headers = headers;
  client->data = data;
  client->size = size;
  reactor_user_construct(&client->user, callback, state);
  reactor_http_client_hold(client);
  reactor_tcp_construct(&client->tcp, reactor_http_client_tcp_event, client, host, port, 0);
}

void reactor_http_client_construct_get(reactor_http_client *client, reactor_user_callback *callback, void *state,
                                       char *host, char *port, char *path)
{
  reactor_http_client_construct(client, callback, state, "GET", host, port, path, NULL, NULL, 0, 0);
}

int reactor_http_client_active(reactor_http_client *client)
{
  return (client->flags & REACTOR_HTTP_CLIENT_FLAG_ACTIVE) != 0;
}

void reactor_http_client_close(reactor_http_client *client)
{
  if (reactor_http_client_active(client))
    {
      client->flags &= ~REACTOR_HTTP_CLIENT_FLAG_ACTIVE;
      if (reactor_tcp_active(&client->tcp))
        reactor_tcp_close(&client->tcp);
      if (reactor_http_active(&client->http))
        reactor_http_close(&client->http);
    }
}
