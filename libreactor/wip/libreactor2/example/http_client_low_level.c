#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

typedef struct client client;
struct client
{
  char         *host;
  char         *port;
  char         *path;
  reactor_tcp   tcp;
  reactor_http  http;
};

void client_http_event(void *state, int type, void *data)
{
  client *client;
  reactor_http_response *response;

  client = state;
  switch (type)
    {
    case REACTOR_HTTP_RESPONSE:
      response = data;
      printf("[status %d]\n", response->status);
      printf("%.*s\n", (int) response->size, (char *) response->data);
      reactor_http_close(&client->http);
      break;
    case REACTOR_HTTP_DESTRUCT:
      break;
    default:
      err(1, "http error");
      break;
    }
}

void client_tcp_event(void *state, int type, void *data)
{
  client *client;

  client = state;
  switch (type)
    {
    case REACTOR_TCP_CONNECT:
      reactor_tcp_close(&client->tcp);
      reactor_http_construct(&client->http, client_http_event, client, *(int *) data, 0);
      reactor_http_send_request(&client->http, (reactor_http_request[]) {{
            .version = 1,
            .method = "GET",
            .path = client->path,
            .headers = {1, (reactor_http_header[]){{"Host", client->host}}, NULL},
            .data = NULL,
            .size = 0
            }});
      reactor_http_flush(&client->http);
      break;
    case REACTOR_TCP_DESTRUCT:
      break;
    default:
      err(1, "tcp error");
    }
}

int main(int argc, char **argv)
{
  client client;
  int e;

  if (argc != 4)
    errx(1, "usage: http_client_low_level <host> <port> <path>\n");

  client.host = argv[1];
  client.port = argv[2];
  client.path = argv[3];

  reactor_core_construct();
  reactor_tcp_construct(&client.tcp, client_tcp_event, &client, client.host, client.port, 0);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
