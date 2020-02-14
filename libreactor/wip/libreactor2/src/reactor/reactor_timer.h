#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

enum reactor_timer_events
{
  REACTOR_TIMER_ERROR,
  REACTOR_TIMER_CALL,
  REACTOR_TIMER_CLOSE,
  REACTOR_TIMER_DESTRUCT
};

enum reactor_timer_flags
{
  REACTOR_TIMER_FLAG_ACTIVE = 0x01,
  REACTOR_TIMER_FLAG_CLONE  = 0x02
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  int           refs;
  int           flags;
  reactor_user  user;
  reactor_desc  desc;
};

void reactor_timer_construct(reactor_timer *, reactor_user_callback *, void *, uint64_t, uint64_t);
void reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
int  reactor_timer_active(reactor_timer *);
void reactor_timer_close(reactor_timer *);

#endif /* REACTOR_TIMER_H_INCLUDED */
