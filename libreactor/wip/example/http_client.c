#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <dynamic.h>
#include <reactor_core.h>
#include "picohttpparser.h"

#ifndef REACTOR_HTTP_CLIENT_HEADER_MAX_FIELDS
#define REACTOR_HTTP_CLIENT_HEADER_MAX_FIELDS 32
#endif

typedef struct reactor_http_client_header_field reactor_http_client_header_field;
struct reactor_http_client_header_field
{
  char                           *key;
  char                           *value;
};

typedef struct reactor_http_client_response reactor_http_client_response;
struct reactor_http_client_response
{
  char                             *base;
  size_t                            size;
  int                               minor_version;
  int                               status;
  char                             *message;
  size_t                            body_size;
  int                               field_count;
  reactor_http_client_header_field  field[REACTOR_HTTP_CLIENT_HEADER_MAX_FIELDS];
  char                             *body;
};

typedef struct reactor_http_client reactor_http_client;
struct reactor_http_client
{
  char                      *name;
  char                      *service;
  char                      *method;
  char                      *path;
  reactor_resolver           resolver;
  struct addrinfo           *addrinfo;
  reactor_stream             stream;
  reactor_http_client_response  response;
};

void reactor_http_client_init(reactor_http_client *, reactor_user_call *, void *);
int  reactor_http_client_open(reactor_http_client *, char *, char *, char *, char *);
void reactor_http_client_stream_event(void *, int, void *);
void reactor_http_client_resolver_event(void *, int, void *);
void reactor_http_client_error(reactor_http_client *);

void reactor_http_client_init(reactor_http_client *http_client, reactor_user_call *call, void *state)
{
  (void) call;
  (void) state;
  *http_client = (reactor_http_client) {0};
  reactor_resolver_init(&http_client->resolver, reactor_http_client_resolver_event, http_client);
  reactor_stream_init(&http_client->stream, reactor_http_client_stream_event, http_client);
}

int reactor_http_client_open(reactor_http_client *http_client, char *name, char *service, char *method, char *path)
{
  int e;

  http_client->name = name;
  http_client->service = service;
  http_client->method = method;
  http_client->path = path;
  e = reactor_resolver_lookup(&http_client->resolver, name, service, NULL);
  if (e == -1)
    return -1;

  return 0;
}

int reactor_http_client_destination_port(reactor_http_client *http_client)
{
  switch (http_client->addrinfo->ai_family)
    {
    case AF_INET:
      return ntohs(((struct sockaddr_in *) http_client->addrinfo->ai_addr)->sin_port);
    case AF_INET6:
      return ntohs(((struct sockaddr_in6 *) http_client->addrinfo->ai_addr)->sin6_port);
    default:
      return 0;
    }
}

void reactor_http_client_request(reactor_http_client *http_client)
{
  int e;

  e = reactor_stream_printf(&http_client->stream,
                            "%s %s HTTP/1.1\r\n"
                            "Host: %s:%d\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            http_client->method, http_client->path, http_client->name,
                            reactor_http_client_destination_port(http_client));
  reactor_stream_flush(&http_client->stream);
  if (e == -1)
    reactor_http_client_error(http_client);
}

void reactor_http_client_data(reactor_http_client *http_client, reactor_stream_data *data)
{
  reactor_http_client_response *response = &http_client->response;
  struct phr_header headers[REACTOR_HTTP_CLIENT_HEADER_MAX_FIELDS];
  size_t headers_count = REACTOR_HTTP_CLIENT_HEADER_MAX_FIELDS;
  size_t message_size;
  int header_size, i;

  if (response->base && data->size < response->size)
    return;

  if (!response->base)
    {
      header_size = phr_parse_response(data->base, data->size, &response->minor_version, &response->status,
                                       (const char **) &response->message, &message_size,
                                       headers, &headers_count, 0);
      if (header_size < 0)
        {
          if (header_size == -1)
            reactor_http_client_error(http_client);
          return;
        }
      response->base = data->base;
      response->message[message_size] = '\0';
      response->field_count = headers_count;
      response->body_size = 0;
      for (i = 0; i < (int) headers_count; i ++)
        {
          response->field[i].key = (char *) headers[i].name;
          response->field[i].key[headers[i].name_len] = '\0';
          response->field[i].value = (char *) headers[i].value;
          response->field[i].value[headers[i].value_len] = '\0';
          if (!response->body_size && strcasecmp(response->field[i].key, "content-length") == 0)
            response->body_size = strtoul(response->field[i].value, NULL, 0);
        }
      response->size = header_size + response->body_size;
      response->body = response->base + header_size;
      if (data->size < response->size)
        return;

      printf("complete %ld %ld\n", data->size, response->size);
    }
}

void reactor_http_client_stream_event(void *state, int type, void *data)
{
  reactor_http_client *http_client;

  (void) data;
  http_client = state;
  (void) http_client;
  switch (type)
    {
    case REACTOR_STREAM_CLOSE:
      break;
    case REACTOR_STREAM_DATA:
      reactor_http_client_data(http_client, data);
      break;
    case REACTOR_STREAM_WRITE_AVAILABLE:
      reactor_http_client_request(http_client);
      break;
    }
}

void reactor_http_client_resolver_event(void *state, int type, void *data)
{
  reactor_http_client *http_client;
  int e;

  http_client = state;
  switch (type)
    {
    case REACTOR_RESOLVER_REPLY:
      http_client->addrinfo = data;
      e = reactor_tcp_client_connect(&http_client->stream, http_client->addrinfo);
      if (e == -1)
        reactor_http_client_error(http_client);
      break;
    }
}

void reactor_http_client_error(reactor_http_client *client)
{
  err(1, "http_client");
}

int main(int argc, char **argv)
{
  reactor_http_client http_client;

  reactor_core_construct();

  reactor_http_client_init(&http_client, NULL, NULL);
  reactor_http_client_open(&http_client, "www.aftonbladet.se", "http", "GET", "/");
  reactor_core_run();
  reactor_core_destruct();
}
