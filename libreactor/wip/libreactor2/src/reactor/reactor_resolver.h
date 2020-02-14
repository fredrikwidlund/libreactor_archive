#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

enum reactor_resolver_events
{
  REACTOR_RESOLVER_ERROR,
  REACTOR_RESOLVER_RESULT,
  REACTOR_RESOLVER_DESTRUCT
};

enum reactor_resolver_flags
{
  REACTOR_RESOLVER_FLAG_ACTIVE = 0x01
};

typedef struct reactor_resolver reactor_resolver;
struct reactor_resolver
{
  int              refs;
  int              flags;
  reactor_user     user;
  reactor_async    async;
  char            *node;
  char            *service;
  struct addrinfo *addrinfo;
  struct addrinfo *hints;
};

void reactor_resolver_construct(reactor_resolver *, reactor_user_callback *, void *, char *, char *, struct addrinfo *);
int  reactor_resolver_active(reactor_resolver *);
void reactor_resolver_close(reactor_resolver *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */
