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

void client_event(void *state, int type, void *data)
{
  reactor_http_client *client;
  reactor_http_response *response;

  client = state;
  switch (type)
    {
    case REACTOR_HTTP_CLIENT_RESPONSE:
      response = data;
      printf("[http_client] response, status %d, reason %s, size %lu\n",
             response->status, response->reason, response->size);
      reactor_http_client_close(client);
      break;
    case REACTOR_HTTP_CLIENT_ERROR:
      fprintf(stderr, "[http_client] error\n");
      reactor_http_client_close(client);
      break;
    default:
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_http_client client;
  int e;

  if (argc != 4)
    errx(1, "usage: http_client <host> <port> <path>\n");

  reactor_core_construct();
  reactor_http_client_construct_get(&client, client_event, &client, argv[1], argv[2], argv[3]);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
