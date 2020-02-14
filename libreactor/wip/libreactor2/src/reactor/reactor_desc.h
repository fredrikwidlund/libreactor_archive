#ifndef REACTOR_DESC_H_INCLUDED
#define REACTOR_DESC_H_INCLUDED

enum reactor_desc_events
{
  REACTOR_DESC_ERROR,
  REACTOR_DESC_READ,
  REACTOR_DESC_WRITE,
  REACTOR_DESC_CLOSE,
  REACTOR_DESC_DESTRUCT
};

enum reactor_desc_flag
{
  REACTOR_DESC_FLAG_ACTIVE = 0x01
};

enum reactor_desc_mask
{
  REACTOR_DESC_MASK_READ = 0x01,
  REACTOR_DESC_MASK_WRITE = 0x04
};

typedef struct reactor_desc reactor_desc;
struct reactor_desc
{
  int          refs;
  int          flags;
  reactor_user user;
  int          fd;
};

void reactor_desc_construct(reactor_desc *, reactor_user_callback *, void *, int, int);
int  reactor_desc_active(reactor_desc *);
int  reactor_desc_close_take(reactor_desc *);
void reactor_desc_close(reactor_desc *);
void reactor_desc_set(reactor_desc *, int);
void reactor_desc_clear(reactor_desc *, int);
int  reactor_desc_fd(reactor_desc *);
void reactor_desc_error(reactor_desc *);
void reactor_desc_event(void *, int, void *);

#endif /* REACTOR_DESC_H_INCLUDED */
