#ifndef REACTOR_REST_H_INCLUDED
#define REACTOR_REST_H_INCLUDED

enum REACTOR_REST_EVENT
{
  REACTOR_REST_ERROR   = 0x01,
  REACTOR_REST_REQUEST = 0x02,
  REACTOR_REST_CLOSE   = 0x04
};

typedef struct reactor_rest reactor_rest;
struct reactor_rest
{
  reactor_user                 user;
  reactor_http_server          http_server;
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
  reactor_http_server_session *session;
  void                        *state;
};

void reactor_rest_init(reactor_rest *, reactor_user_call *, void *);
int  reactor_rest_open(reactor_rest *, char *, char *);
void reactor_rest_event(void *, int, void *);
int  reactor_rest_add(reactor_rest *, char *, char *, void *);
int  reactor_rest_match(reactor_rest_map *, reactor_http_server_request *);
void reactor_rest_return(reactor_rest_request *, int, char *, char *, char *, size_t);
void reactor_rest_return_not_found(reactor_rest_request *);
void reactor_rest_return_text(reactor_rest_request *, char *);

#endif /* REACTOR_REST_H_INCLUDED */
