#ifndef REACTOR_CORE_H_INCLUDED
#define REACTOR_CORE_H_INCLUDED

enum reactor_core_fd_event
{
  REACTOR_CORE_FD_EVENT_ERROR,
  REACTOR_CORE_FD_EVENT_READ,
  REACTOR_CORE_FD_EVENT_WRITE,
  REACTOR_CORE_FD_EVENT_HANGUP
};

enum reactor_core_fd_mask
{
  REACTOR_CORE_FD_MASK_READ  = 0x01, /* POLLIN */
  REACTOR_CORE_FD_MASK_WRITE = 0x04  /* POLLOUT */
};

void  reactor_core_fd_register(int, reactor_user_callback *, void *, short);
void  reactor_core_fd_deregister(int);
void  reactor_core_fd_set(int, short);
void  reactor_core_fd_clear(int, short);
void  reactor_core_job_register(reactor_user_callback *, void *);
void  reactor_core_construct();
void  reactor_core_destruct();
int   reactor_core_run();

#endif /* REACTOR_CORE_H_INCLUDED */
