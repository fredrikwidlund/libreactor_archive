#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/timerfd.h>

#include <cmocka.h>

int mock_poll_failure = 0;
int __real_poll(struct pollfd *, nfds_t, int);
int __wrap_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
  return mock_poll_failure ? -1 : __real_poll(fds, nfds, timeout);
}

int mock_read_failure = 0;
ssize_t __real_read(int, void *, size_t);
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
  if (!mock_read_failure)
    return __real_read(fildes, buf, nbyte);

  errno = mock_read_failure;
  mock_read_failure = 0;
  return -1;
}

int mock_write_failure = 0;
ssize_t __real_write(int, void *, size_t);
ssize_t __wrap_write(int fildes, void *buf, size_t nbyte)
{
  if (!mock_write_failure)
    return __real_write(fildes, buf, nbyte);

  errno = mock_write_failure;
  mock_write_failure = 0;
  return -1;
}

int mock_timerfd_failure = 0;
int __real_timerfd_create(int, int);
int __wrap_timerfd_create(int clockid, int flags)
{
  if (!mock_timerfd_failure)
    return __real_timerfd_create(clockid, flags);
  errno = mock_write_failure;
  mock_timerfd_failure = 0;
  return -1;
}

int __real_timerfd_settime(int, int, const struct itimerspec *, struct itimerspec *);
int __wrap_timerfd_settime(int fd, int flags,
                           const struct itimerspec *new_value,
                           struct itimerspec *old_value)
{
  if (!mock_timerfd_failure)
    return __real_timerfd_settime(fd, flags, new_value, old_value);
  mock_timerfd_failure = 0;
  return -1;
}
