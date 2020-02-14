#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

enum reactor_core_events
{
  REACTOR_CORE_POLL
};

enum reactor_core_state
{
  REACTOR_CORE_ACTIVE,
  REACTOR_CORE_DEACTIVATING
};

void reactor_core_construct();
void reactor_core_destruct();
int  reactor_core_add(int, reactor_user_callback *, void *, int);
void reactor_core_remove(int);
int  reactor_core_get(int);
void reactor_core_set(int, int);
void reactor_core_clear(int, int);
int  reactor_core_run();

#endif /* REACTOR_CORE_H_INCLUDED */
