#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "url.h"

static const char *url_regex = "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?";

static char *url_match(char *string, regmatch_t *match, size_t i, char *def)
{
  char *result;
  size_t len;

  result = def;
  len = match[i].rm_eo - match[i].rm_so;
  if (len)
    {
      result = malloc(len + 1);
      memcpy(result, string + match[i].rm_so, len);
      result[len] = 0;
    }
  return result;
}

void url_construct(url *url, char *string)
{
  regex_t reg;
  regmatch_t match[10];
  int e;

  *url = (struct url) {0};
  (void) regcomp(&reg, url_regex, REG_EXTENDED);
  e = regexec(&reg, string, 10, match, 0);
  if (e == 0)
    {
      url->scheme = url_match(string, match, 2, "http");
      url->host = url_match(string, match, 4, "localhost");
      url->port = "80";
      url->path = url_match(string, match, 5, "/");
      url->query = url_match(string, match, 7, "");
      url->fragment = url_match(string, match, 9, "");
    }

  regfree(&reg);
}

void url_destruct(url *url)
{
  (void) url;
}

int url_valid(url *url)
{
  return (strcmp(url_scheme(url), "http") == 0) || (strcmp(url_scheme(url), "https") == 0);
}

char *url_scheme(url *url)
{
  return url->scheme;
}

char *url_host(url *url)
{
  return url->host;
}

char *url_port(url *url)
{
  return url->port;
}

char *url_path(url *url)
{
  return url->path;
}
