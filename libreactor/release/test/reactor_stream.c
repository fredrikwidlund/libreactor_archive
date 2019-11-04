#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/param.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int mock_read_failure;

static int reader_data = 0;
static int reader_read = 0;
static int reader_write = 0;
static int reader_hangup = 0;
static int reader_error = 0;
static int reader_close = 0;

void core_reader(void *state, int type, void *arg)
{
  reactor_stream *stream = state;
  reactor_memory *data = arg;

  switch(type)
    {
    case REACTOR_STREAM_EVENT_READ:
      reader_data += data->size;
      data->size = 0;
      reader_read ++;
      break;
    case REACTOR_STREAM_EVENT_WRITE_UNBLOCKED:
      reader_write ++;
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      reader_hangup ++;
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_EVENT_ERROR:
      reader_error ++;
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      reader_close ++;
      break;
    }
}

void core_writer(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_ERROR:
      break;
    case REACTOR_STREAM_EVENT_WRITE_UNBLOCKED:
      reactor_stream_close(stream);
      break;
    }
}

void core()
{

    reactor_stream reader, writer;
  int e, fds[2];

  reactor_core_construct();

  e = pipe(fds);
  assert_int_equal(e, 0);
  reactor_stream_open(&reader, core_reader, &reader, fds[0]);
  reactor_stream_open(&writer, core_writer, &writer, fds[1]);
  reactor_stream_hold(&writer);
  reactor_stream_write(&writer, "test1", 5);
  reactor_stream_write(&writer, "test2", 5);
  reactor_stream_write(&writer, "test3", 5);
  reactor_stream_close(&writer);
  reactor_stream_release(&writer);

  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(reader_data, 15);

  reader_error = 0;
  reactor_stream_open(&reader, core_reader, &reader, 1000);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(reader_error, 1);

reactor_stream_open(&writer, core_writer, &writer, 1000);
  reactor_stream_write(&writer, "test", 4);
  reactor_stream_flush(&writer);
  reactor_stream_close(&writer);

  reactor_stream_open(&writer, core_writer, &writer, 1000);
  reactor_stream_write(&writer, "test", 4);
  reactor_stream_close(&writer);

  reactor_core_destruct();
}

typedef struct pump pump;
struct pump
{
  int blocked;
  reactor_stream writer;
  reactor_stream reader;
  size_t write;
  size_t read;
};

void pump_write(pump *p)
{
  char buffer[1024*1024] = {0};
  size_t n;

  while (!p->blocked && p->write)
    {
      n = MIN(p->write, sizeof buffer);
      p->write -= n;
      reactor_stream_write(&p->writer, buffer, n);
      reactor_stream_flush(&p->writer);
    }
  if (!p->write)
    reactor_stream_close(&p->writer);
}

void pump_event(void *state, int type, void *data)
{
  pump *p = state;
  reactor_memory *m;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      m = data;
      if (m->size == p->read || m->size > 1024L * 1024L || p->read < 1024L * 1024L)
        {
          p->read -= m->size;
          reactor_stream_consume(&p->reader, m->size);
        }
      break;
    case REACTOR_STREAM_EVENT_WRITE_UNBLOCKED:
      p->blocked = 0;
      pump_write(p);
      break;
    case REACTOR_STREAM_EVENT_WRITE_BLOCKED:
      p->blocked = 1;
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_stream_close(&p->reader);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      break;
    }
}

void pump_test()
{
  pump p = {0};
  int e, fds[2];

  reactor_core_construct();

  e = pipe(fds);
  assert_int_equal(e, 0);

  p.write = 128L * 1024L * 1024L;
  p.read = p.write;
  reactor_stream_open(&p.reader, pump_event, &p, fds[0]);
  reactor_stream_open(&p.writer, pump_event, &p, fds[1]);
  pump_write(&p);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(p.read == 0);
  assert_true(p.write == 0);

  reactor_core_destruct();
}

void edge_event(void *state, int type, void *data)
{
  reactor_stream *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_ERROR:
      reactor_stream_close(s);
      break;
    }
}

void edge()
{
  reactor_stream s;
  int e, fds[2];
  void reactor_stream_event(void *, int, void *);
  extern int mock_write_failure;

  reactor_core_construct();

  // open invalid descriptor
  reactor_stream_open(&s, edge_event, &s, -1);

  // open unused descriptor
  reactor_stream_open(&s, edge_event, &s, 1000);

  // stream from closed descriptor
  e = pipe(fds);
  assert_int_equal(e, 0);
  reactor_stream_open(&s, edge_event, &s, fds[0]);
  close(fds[0]);
  close(fds[1]);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  // undefined event
  reactor_stream_event(NULL, -1, NULL);

  // hold and release
  reactor_stream_open(&s, edge_event, &s, 0);
  reactor_stream_hold(&s);
  reactor_stream_close(&s);
  reactor_stream_release(&s);

  // close closed stream
  reactor_stream_close(&s);

  // flush closed stream
  reactor_stream_flush(&s);

  // write error
  e = pipe(fds);
  assert_int_equal(e, 0);
  reactor_stream_open(&s, edge_event, &s, fds[1]);
  mock_write_failure = 1;
  reactor_stream_write(&s, "test", 4);
  reactor_stream_flush(&s);
  close(fds[0]);

  // flush empty stream
  e = pipe(fds);
  assert_int_equal(e, 0);
  reactor_stream_open(&s, edge_event, &s, fds[0]);
  reactor_stream_flush(&s);
  reactor_stream_close(&s);
  close(fds[1]);

  reactor_core_destruct();
}

void file_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_ERROR:
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_stream_close(stream);
      break;
    }
}

void file()
{
  int fd, e;
  reactor_stream reader;

  reactor_core_construct();

  // file eof
  fd = open("/dev/null", O_RDONLY);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_event, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  // read error
  fd = open("/dev/null", O_RDONLY);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_event, &reader, fd);
  mock_read_failure = EIO;
  e = reactor_core_run();
  assert_int_equal(e, 0);

  // read would block
  fd = open("/dev/null", O_RDONLY);
  assert_true(fd != -1);
  mock_read_failure = EAGAIN;
  reactor_stream_open(&reader, file_event, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  // close on already closed stream
  //  reactor_stream_close(&reader);

  reactor_core_destruct();
}

void sock()
{
  int fd, e;
  reactor_stream reader;

  reactor_core_construct();

  fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_event, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(pump_test),
    cmocka_unit_test(edge),
    cmocka_unit_test(file),
    //cmocka_unit_test(sock)
    //cmocka_unit_test(core),
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
