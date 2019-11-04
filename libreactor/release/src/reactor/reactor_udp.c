#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "reactor_udp.h"

static void reactor_udp_error(reactor_udp *udp)
{
  udp->state = REACTOR_UDP_STATE_ERROR;
  reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_ERROR, NULL);
}

static void reactor_udp_connect(reactor_udp *udp, struct addrinfo *addrinfo)
{
  (void) udp;
  (void) addrinfo;
}

static void reactor_udp_listen(reactor_udp *udp, struct addrinfo *addrinfo)
{
  int e, s;
  struct sockaddr_in sin;

  s = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (s == -1)
    {
      reactor_udp_error(udp);
      return;
    }

  e = fcntl(s, F_SETFL, O_NONBLOCK);
  if (e == -1)
    {
      (void) close(s);
      reactor_udp_error(udp);
      return;
    }

  if (addrinfo->ai_family == AF_INET)
    {
      sin = *(struct sockaddr_in *) addrinfo->ai_addr;
      if (IN_MULTICAST(ntohl(sin.sin_addr.s_addr)))
	{

	  (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
	  e = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			 (struct ip_mreq[]){{.imr_multiaddr = sin.sin_addr, .imr_interface.s_addr = htonl(INADDR_ANY)}},
			 sizeof(struct ip_mreq));
	  if (e == -1)
	    {
	      (void) close(s);
	      reactor_udp_error(udp);
	      return;
	    }
	}
    }
  
  e = bind(s, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == -1)
    {
      (void) close(s);
      reactor_udp_error(udp);
      return;
    }

  reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_SOCKET, &s);
}

static void reactor_udp_resolve_event(void *state, int type, void *data)
{
  reactor_udp *udp = state;

  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_RESULT:
      if (!data)
        reactor_udp_error(udp);
      else if (udp->flags & REACTOR_UDP_FLAG_SERVER)
        reactor_udp_listen(udp, data);
      else
        reactor_udp_connect(udp, data);
      break;
    case REACTOR_RESOLVER_EVENT_ERROR:
      reactor_udp_error(udp);
      break;
    case REACTOR_RESOLVER_EVENT_CLOSE:
      free(udp->resolver);
      udp->resolver = NULL;
      reactor_udp_release(udp);
      break;
    }
}

static void reactor_udp_resolve(reactor_udp *udp, char *node, char *service)
{
  reactor_udp_hold(udp);
  udp->resolver = malloc(sizeof *udp->resolver);
  reactor_resolver_open(udp->resolver, reactor_udp_resolve_event, udp, node, service,
                        (struct addrinfo[]){{.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM}});
}

void reactor_udp_hold(reactor_udp *udp)
{
  udp->ref ++;
}

void reactor_udp_release(reactor_udp *udp)
{
  udp->ref --;
  if (!udp->ref)
    {
      udp->state = REACTOR_UDP_STATE_CLOSED;
      reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_CLOSE, NULL);
    }
}

void reactor_udp_open(reactor_udp *udp, reactor_user_callback *callback, void *state, char *node, char *service, int flags)
{
  udp->ref = 0;
  udp->state = REACTOR_UDP_STATE_RESOLVING;
  reactor_user_construct(&udp->user, callback, state);
  udp->flags = flags;
  reactor_udp_hold(udp);
  reactor_udp_resolve(udp, node, service);
}

void reactor_udp_close(reactor_udp *udp)
{
  if (udp->state & REACTOR_UDP_STATE_CLOSED)
    return;

  udp->state = REACTOR_UDP_STATE_CLOSED;
  reactor_udp_release(udp);
}

