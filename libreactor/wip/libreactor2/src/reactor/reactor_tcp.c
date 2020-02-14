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
#include "url.h"
#include "reactor.h"

static void reactor_tcp_hold(reactor_tcp *tcp)
{
  tcp->refs ++;
}

static void reactor_tcp_release(reactor_tcp *tcp)
{
  tcp->refs --;
  if (!tcp->refs)
    {
      free(tcp->node);
      free(tcp->service);
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_DESTRUCT, tcp);
    }
}

static void reactor_tcp_error(reactor_tcp *tcp)
{
  reactor_user_dispatch(&tcp->user, REACTOR_TCP_ERROR, tcp);
}

static void reactor_tcp_desc_event(void *state, int type, void *data)
{
  reactor_tcp *tcp;
  int fd;

  (void) data;
  tcp = state;
  reactor_tcp_hold(tcp);
  switch (type)
    {
    case REACTOR_DESC_READ:
      fd = accept(reactor_desc_fd(&tcp->desc), NULL, NULL);
      if (fd >= 0)
        reactor_user_dispatch(&tcp->user, REACTOR_TCP_ACCEPT, &fd);
      else
        reactor_tcp_error(tcp);
      break;
    case REACTOR_DESC_WRITE:
      fd = reactor_desc_close_take(&tcp->desc);
      reactor_user_dispatch(&tcp->user, REACTOR_TCP_CONNECT, &fd);
      break;
    case REACTOR_DESC_ERROR:
    case REACTOR_DESC_CLOSE:
      reactor_desc_close(&tcp->desc);
      reactor_tcp_error(tcp);
      break;
    case REACTOR_DESC_DESTRUCT:
      reactor_tcp_release(tcp);
      break;
    }
  reactor_tcp_release(tcp);
}

static void reactor_tcp_listen(reactor_tcp *tcp, struct addrinfo *addrinfo)
{
  int e, s;

  if (!addrinfo)
    {
      reactor_tcp_error(tcp);
      return;
    }

  s = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (s == -1)
    {
      reactor_tcp_error(tcp);
      return;
    }
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));

  e = bind(s, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  e = listen(s, -1);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  reactor_tcp_hold(tcp);
  reactor_desc_construct(&tcp->desc, reactor_tcp_desc_event, tcp, s, REACTOR_DESC_MASK_READ);
}

static void reactor_tcp_connect(reactor_tcp *tcp, struct addrinfo *addrinfo)
{
  int e, s;

  if (!addrinfo)
    {
      reactor_tcp_error(tcp);
      return;
    }

  s = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (s == -1)
    {
      reactor_tcp_error(tcp);
      return;
    }

  e = fcntl(s, F_SETFL, O_NONBLOCK);
  if (e == -1)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  e = connect(s, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == -1 && errno != EINPROGRESS)
    {
      (void) close(s);
      reactor_tcp_error(tcp);
      return;
    }

  reactor_tcp_hold(tcp);
  reactor_desc_construct(&tcp->desc, reactor_tcp_desc_event, tcp, s, REACTOR_DESC_MASK_WRITE);
}

static void reactor_tcp_resolver_event(void *state, int type, void *data)
{
  reactor_tcp *tcp;
  struct addrinfo *addrinfo;

  tcp = state;
  switch (type)
    {
    case REACTOR_RESOLVER_RESULT:
      addrinfo = data;
      (tcp->flags & REACTOR_TCP_FLAG_SERVER ? reactor_tcp_listen : reactor_tcp_connect)(tcp, addrinfo);
      break;
    case REACTOR_RESOLVER_ERROR:
      reactor_tcp_error(tcp);
      break;
    case REACTOR_RESOLVER_DESTRUCT:
      reactor_tcp_release(tcp);
      break;
    }
}

void reactor_tcp_construct(reactor_tcp *tcp, reactor_user_callback *callback, void *state,
                           char *node, char *service, int flags)
{
  *tcp = (reactor_tcp) {.flags = REACTOR_TCP_FLAG_ACTIVE | flags};
  reactor_tcp_hold(tcp);
  reactor_user_construct(&tcp->user, callback, state);
  reactor_tcp_hold(tcp);
  reactor_resolver_construct(&tcp->resolver, reactor_tcp_resolver_event, tcp, node, service,
                             (struct addrinfo[]){{.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM,
                                   .ai_flags = flags & REACTOR_TCP_FLAG_SERVER ? AI_PASSIVE : 0}});
}

int reactor_tcp_active(reactor_tcp *tcp)
{
  return (tcp->flags & REACTOR_TCP_FLAG_ACTIVE) != 0;
}

void reactor_tcp_close(reactor_tcp *tcp)
{
  tcp->flags &= ~REACTOR_TCP_FLAG_ACTIVE;
  if (reactor_resolver_active(&tcp->resolver))
    reactor_resolver_close(&tcp->resolver);
  if (reactor_desc_active(&tcp->desc))
    reactor_desc_close(&tcp->desc);
  reactor_tcp_release(tcp);
}
