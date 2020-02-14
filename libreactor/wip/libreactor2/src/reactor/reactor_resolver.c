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

static void reactor_resolver_hold(reactor_resolver *resolver)
{
  resolver->refs ++;
}

static void reactor_resolver_release(reactor_resolver *resolver)
{
  resolver->refs --;
  if (!resolver->refs)
    {
      freeaddrinfo(resolver->addrinfo);
      free(resolver->node);
      free(resolver->service);
      free(resolver->hints);
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_DESTRUCT, resolver);
    }
}

static void reactor_resolver_error(reactor_resolver *resolver)
{
  reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_ERROR, resolver);
}

static void reactor_resolver_event(void *state, int type, void *data)
{
  reactor_resolver *resolver;

  (void) data;
  resolver = state;
  reactor_resolver_hold(resolver);
  switch (type)
    {
    case REACTOR_ASYNC_CALL:
      resolver->addrinfo = NULL;
      (void) getaddrinfo(resolver->node, resolver->service, resolver->hints, &resolver->addrinfo);
      break;
    case REACTOR_ASYNC_ERROR:
      reactor_async_close(&resolver->async);
      reactor_resolver_error(resolver);
      break;
    case REACTOR_ASYNC_CLOSE:
      reactor_async_close(&resolver->async);
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_RESULT, resolver->addrinfo);
      break;
    case REACTOR_ASYNC_DESTRUCT:
      reactor_resolver_release(resolver);
      break;
    }
  reactor_resolver_release(resolver);
}

void reactor_resolver_construct(reactor_resolver *resolver, reactor_user_callback *callback, void *state,
                                char *node, char *service, struct addrinfo *hints)
{
  int flags_saved;

  *resolver = (reactor_resolver) {.flags = REACTOR_RESOLVER_FLAG_ACTIVE};
  reactor_resolver_hold(resolver);
  reactor_user_construct(&resolver->user, callback, state);
  resolver->hints = malloc(sizeof *hints);
  *resolver->hints = hints ? *hints : (struct addrinfo){.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  resolver->node = strdup(node);
  resolver->service = strdup(service);

  flags_saved = resolver->hints->ai_flags;
  resolver->hints->ai_flags |= AI_NUMERICHOST | AI_NUMERICSERV;
  (void) getaddrinfo(node, service, resolver->hints, &resolver->addrinfo);
  if (resolver->addrinfo)
    {
      reactor_user_dispatch(&resolver->user, REACTOR_RESOLVER_RESULT, resolver->addrinfo);
      reactor_resolver_release(resolver);
    }
  else
    {
      resolver->hints->ai_flags = flags_saved;
      reactor_async_construct(&resolver->async, reactor_resolver_event, resolver);
    }
}

int reactor_resolver_active(reactor_resolver *resolver)
{
  return (resolver->flags & REACTOR_RESOLVER_FLAG_ACTIVE) != 0;
}

void reactor_resolver_close(reactor_resolver *resolver)
{
  resolver->flags &= ~REACTOR_RESOLVER_FLAG_ACTIVE;
  if (reactor_async_active(&resolver->async))
    reactor_async_close(&resolver->async);
  reactor_resolver_release(resolver);
}
