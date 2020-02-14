#ifndef REACTOR_TCP_H_INCLUDED
#define REACTOR_TCP_H_INCLUDED

enum reactor_tcp_events
{
  REACTOR_TCP_ERROR,
  REACTOR_TCP_ACCEPT,
  REACTOR_TCP_CONNECT,
  REACTOR_TCP_CLOSE,
  REACTOR_TCP_DESTRUCT
};

enum reactor_tcp_flags
{
  REACTOR_TCP_FLAG_ACTIVE = 0x01,
  REACTOR_TCP_FLAG_SERVER = 0x02
};

typedef struct reactor_tcp reactor_tcp;
struct reactor_tcp
{
  int               refs;
  int               flags;
  reactor_user      user;
  char             *node;
  char             *service;
  reactor_resolver  resolver;
  struct addrinfo  *addrinfo;
  reactor_desc      desc;
};

void reactor_tcp_construct(reactor_tcp *, reactor_user_callback *, void *, char *, char *, int);
int  reactor_tcp_active(reactor_tcp *);
void reactor_tcp_close(reactor_tcp *);

#endif /* REACTOR_TCP_H_INCLUDED */
