#include <stdlib.h>
#include <stdint.h>

#include "dynamic.h"
#include "url.h"
#include "reactor.h"

void reactor_user_construct(reactor_user *user, reactor_user_callback *callback, void *state)
{
  *user = (reactor_user) {.callback = callback, .state = state};
}

void reactor_user_dispatch(reactor_user *user, int type, void *data)
{
  user->callback(user->state, type, data);
}
