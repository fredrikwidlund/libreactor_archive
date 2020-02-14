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
#include "reactor.h"

typedef struct timeout_state timeout_state;
struct timeout_state
{
  reactor_timer                timer;
  reactor_http_server_session *session;
};

void hello(void *state __attribute__ ((unused)), int type __attribute__ ((unused)), void *data)
{
  reactor_http_server_session_respond_text(data, "Hello, World!");
}

static int count_malloc = 0;
static int count_free = 0;
static int count_timeout = 0;

void timeout_event(void *state, int type, void *data)
{
  timeout_state *ts = state;

  (void) data;
  switch (type)
    {
    case REACTOR_TIMER_CALL:
      count_timeout ++;
      printf("timeout %d\n", count_timeout);
      reactor_http_server_session_respond_text(ts->session, "Timeout");
      reactor_timer_close(&ts->timer);
      break;
    case REACTOR_TIMER_CLOSE:
    case REACTOR_TIMER_ERROR:
      printf("error\n");
      reactor_timer_close(&ts->timer);
      break;
    case REACTOR_TIMER_DESTRUCT:
      count_free ++;
      printf("free %d/%d\n", count_free, count_malloc);
      free(ts);
      break;
    }
}

void timeout(void *state, int type, void *data)
{
  timeout_state *ts;

  (void) state;
  (void) type;
  count_malloc ++;
  printf("malloc %d (%d)\n", count_malloc, type);
  ts = malloc(sizeof *ts);
  ts->session = data;
  reactor_timer_construct(&ts->timer, timeout_event, ts, 1000000000, 0);
}

int main(int argc, char **argv)
{
  reactor_http_server server;
  int e;

  (void) argv;
  if (argc != 1)
    errx(1, "usage: http_server\n");

  signal(SIGPIPE, SIG_IGN);

  reactor_core_construct();
  reactor_http_server_construct(&server, NULL, &server, "127.0.0.1", "80", 0);
  reactor_http_server_name(&server, "*");
  reactor_http_server_match(&server, hello, &server, "GET", "/");
  reactor_http_server_match(&server, timeout, &server, "GET", "/timeout");
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
