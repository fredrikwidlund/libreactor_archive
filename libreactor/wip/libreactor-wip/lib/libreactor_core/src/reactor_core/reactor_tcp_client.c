#define _GNU_SOURCE

#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"

int reactor_tcp_client_connect(reactor_stream *stream, struct addrinfo *ai)
{
  int fd, e;

  fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (fd == -1)
    return -1;

  e = reactor_stream_open(stream, fd);
  if (e == -1)
    {
      (void) close(fd);
      return -1;
    }

  e = connect(fd, ai->ai_addr, ai->ai_addrlen);
  if (e == -1 && errno != EINPROGRESS)
    return -1;

  return 0;
}
