#ifndef REACTOR_ASYNC_H_INCLUDED
#define REACTOR_ASYNC_H_INCLUDED

#ifndef REACTOR_ASYNC_STACK_SIZE
#define REACTOR_ASYNC_STACK_SIZE 1048576
#endif

enum reactor_async_events
{
  REACTOR_ASYNC_ERROR,
  REACTOR_ASYNC_CALL,
  REACTOR_ASYNC_CLOSE,
  REACTOR_ASYNC_DESTRUCT
};

enum reactor_async_flags
{
  REACTOR_ASYNC_FLAG_ACTIVE = 0x01,
  REACTOR_ASYNC_FLAG_CLONE  = 0x02
};

typedef struct reactor_async reactor_async;
struct reactor_async
{
  int           refs;
  int           flags;
  reactor_user  user;
  reactor_desc  desc;
  void         *stack;
  int           fd[2];
  pid_t         pid;
};

void reactor_async_construct(reactor_async *, reactor_user_callback *, void *);
int  reactor_async_active(reactor_async *);
void reactor_async_close(reactor_async *);

#endif /* REACTOR_ASYNC_H_INCLUDED */
