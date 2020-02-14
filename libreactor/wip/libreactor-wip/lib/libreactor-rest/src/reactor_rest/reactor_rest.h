#ifndef REACTOR_REST_H_INCLUDED
#define REACTOR_REST_H_INCLUDED

enum
{
  REACTOR_REST_ERROR = 0,
  REACTOR_REST_REQUEST,
  REACTOR_REST_CLOSE
};

typedef struct reactor_rest reactor_rest;
struct reactor_rest
{
  reactor_http_server          http_server;
  reactor_user                 user;
  vector                       maps;
};

typedef struct reactor_rest_map reactor_rest_map;
struct reactor_rest_map
{
  char                        *method;
  char                        *path;
  void                        *state;
};

typedef struct reactor_rest_request reactor_rest_request;
struct reactor_rest_request
{
  reactor_http_server_request *http_request;
  void                        *state;
};

void reactor_rest_init(reactor_rest *, reactor_user_call *, void *);
int  reactor_rest_open(reactor_rest *, char *, char *);
void reactor_rest_event(void *, int, void *);
int  reactor_rest_add(reactor_rest *, char *, char *, void *);
int  reactor_rest_map_match(reactor_rest_map *, reactor_http_server_request *);
int  reactor_rest_match(char *, const char *, size_t);
void reactor_rest_response_text(reactor_rest_request *, char *);
void reactor_rest_response(reactor_rest_request *, char *, char *, size_t);

#endif /* REACTOR_REST_H_INCLUDED */
