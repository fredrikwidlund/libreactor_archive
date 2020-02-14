#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <err.h>
#include <sys/socket.h>

#include <dynamic.h>
#include <reactor.h>
#include <reactor_http.h>
#include <reactor_rest.h>

// gcc -Wall -Wpedantic -I/usr/local/include -O3 -o test test.c -march=native -flto -fuse-linker-plugin -lreactor-rest -lreactor-http -lreactor -ldynamic 

typedef struct map_state map_state;
struct map_state
{
  void (*callback)(reactor_rest_request *);
};

void health(reactor_rest_request *request)
{
  reactor_rest_response_text(request, "OK\n");
}

void event(void *state, int type, void *data)
{
  reactor_rest_request *request = data;
  map_state *map_state = request->state;
  
  map_state->callback(request);
}

int main()
{
  reactor_core core;
  reactor_rest server;
  
  reactor_core_init(&core);
  reactor_rest_init(&server, event, NULL);
  reactor_rest_add(&server, "GET", "/health", (map_state[]) {{.callback = health}});
  assert(reactor_rest_open(&server, NULL, NULL) == 0);
  assert(reactor_core_run(&core) == 0);
  reactor_core_delete(&core);
}

