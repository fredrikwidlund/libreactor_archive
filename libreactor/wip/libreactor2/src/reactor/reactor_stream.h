#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#ifndef REACTOR_STREAM_BLOCK_SIZE
#define REACTOR_STREAM_BLOCK_SIZE 65536
#endif

enum reactor_stream_events
{
  REACTOR_STREAM_ERROR,
  REACTOR_STREAM_READ,
  REACTOR_STREAM_WRITE,
  REACTOR_STREAM_BLOCKED,
  REACTOR_STREAM_CLOSE,
  REACTOR_STREAM_DESTRUCT
};

enum reactor_stream_flags
{
  REACTOR_STREAM_FLAG_ACTIVE  = 0x01,
  REACTOR_STREAM_FLAG_BLOCKED = 0x02,
  REACTOR_STREAM_FLAG_LINGER  = 0x04
};

typedef struct reactor_stream reactor_stream;
struct reactor_stream
{
  int           refs;
  int           flags;
  reactor_user  user;
  reactor_desc  desc;
  buffer        read;
  buffer        write;
};

typedef struct reactor_stream_data reactor_stream_data;
struct reactor_stream_data
{
  void         *base;
  size_t        size;
};

void    reactor_stream_construct(reactor_stream *, reactor_user_callback *, void *, int);
int     reactor_stream_active(reactor_stream *);
void    reactor_stream_close(reactor_stream *);
void    reactor_stream_write(reactor_stream *, void *, size_t);
void    reactor_stream_write_string(reactor_stream *, char *);
void    reactor_stream_write_unsigned(reactor_stream *, uint32_t);
void    reactor_stream_flush(reactor_stream *);
void    reactor_stream_set_blocked(reactor_stream *);
void    reactor_stream_clear_blocked(reactor_stream *);
int     reactor_stream_blocked(reactor_stream *);

void   *reactor_stream_data_base(reactor_stream_data *);
size_t  reactor_stream_data_size(reactor_stream_data *);
void    reactor_stream_data_consume(reactor_stream_data *, size_t);

#endif /* REACTOR_STREAM_H_INCLUDED */

