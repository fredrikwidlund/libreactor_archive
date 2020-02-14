#ifndef URL_H_INCLUDED
#define URL_H_INCLUDED

typedef struct url url;
struct url
{
  char *scheme;
  char *host;
  char *port;
  char *path;
  char *query;
  char *fragment;
};

void  url_construct(url *, char *);
void  url_destruct(url *);
int   url_valid(url *);
char *url_scheme(url *);
char *url_host(url *);
char *url_port(url *);
char *url_path(url *);

#endif /* URL_H_INCLUDED */
