#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"

static int reactor_pool_worker_thread(void *arg)
{
  reactor_pool *pool = arg;
  reactor_pool_job *job;
  ssize_t n;

  while (1)
    {
      n = read(pool->queue[1], &job, sizeof job);
      if (n != sizeof job)
        return 1;
      reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_CALL, NULL);
      n = write(pool->queue[1], &job, sizeof job);
      if (n != sizeof job)
        return 1;
    }

  return 0;
}

static void reactor_pool_grow(reactor_pool *pool)
{
  reactor_pool_worker *worker;

  if (pool->workers >= pool->workers_max)
    return;

  worker = malloc(sizeof *worker);
  worker->stack = malloc(REACTOR_POOL_STACK_SIZE);
  worker->pid = clone(reactor_pool_worker_thread, (char *) worker->stack + REACTOR_POOL_STACK_SIZE,
                      CLONE_VM | CLONE_FS | CLONE_SIGHAND | CLONE_PARENT,
                      pool);
  TAILQ_INSERT_TAIL(&pool->workers_head, worker, entries);
  pool->workers ++;
}

static void reactor_pool_dequeue(reactor_pool *pool)
{
  reactor_pool_job *job;
  ssize_t n;

  n = read(pool->queue[0], &job, sizeof job);
  if (n != sizeof job)
    return;
  reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_RETURN, NULL);
  free(job);
  pool->jobs --;
  if (!pool->jobs)
    reactor_core_fd_deregister(pool->queue[0]);
}

static void reactor_pool_flush(reactor_pool *pool)
{
  reactor_pool_job *job;
  ssize_t n;

  while (!TAILQ_EMPTY(&pool->jobs_head))
    {
      job = TAILQ_FIRST(&pool->jobs_head);
      n = write(pool->queue[0], &job, sizeof job);
      if (n != sizeof job)
        break;
      TAILQ_REMOVE(&pool->jobs_head, job, entries);
    }

  if (TAILQ_EMPTY(&pool->jobs_head))
    reactor_core_fd_clear(pool->queue[0], REACTOR_CORE_FD_MASK_WRITE);
  else
    reactor_core_fd_set(pool->queue[0], REACTOR_CORE_FD_MASK_WRITE);
}

static void reactor_pool_event(void *state, int type, void *data)
{
  reactor_pool *pool = state;

  (void) data;
  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      reactor_pool_dequeue(pool);
      break;
    case REACTOR_CORE_FD_EVENT_WRITE:
      reactor_pool_flush(pool);
      break;
    default:
      reactor_core_fd_deregister(pool->queue[0]);
      break;
    }
}

void reactor_pool_construct(reactor_pool *pool)
{
  TAILQ_INIT(&pool->jobs_head);
  pool->jobs = 0;
  TAILQ_INIT(&pool->workers_head);
  pool->workers = 0;
  pool->workers_min = 0;
  pool->workers_max = REACTOR_POOL_WORKERS_MAX;
  (void) socketpair(PF_UNIX, SOCK_DGRAM, 0, pool->queue);
  (void) fcntl(pool->queue[0], F_SETFL, O_NONBLOCK);
}

void reactor_pool_destruct(reactor_pool *pool)
{
  reactor_pool_worker *worker;
  reactor_pool_job *job;

  while (!TAILQ_EMPTY(&pool->workers_head))
    {
      worker = TAILQ_FIRST(&pool->workers_head);
      TAILQ_REMOVE(&pool->workers_head, worker, entries);
      pool->workers --;
      kill(worker->pid, SIGTERM);
      waitpid(worker->pid, NULL, 0);
      free(worker->stack);
      free(worker);
    }

  while (!TAILQ_EMPTY(&pool->jobs_head))
    {
      job = TAILQ_FIRST(&pool->jobs_head);
      TAILQ_REMOVE(&pool->jobs_head, job, entries);
      pool->jobs --;
      free(job);
    }

  if (pool->queue[0] >= 0)
    {
      (void) close(pool->queue[0]);
      pool->queue[0] = -1;
    }
  if (pool->queue[1] >= 0)
    {
      (void) close(pool->queue[1]);
      pool->queue[1] = -1;
    }
}

void reactor_pool_limits(reactor_pool *pool, size_t min, size_t max)
{
  pool->workers_min = min;
  pool->workers_max = max;
  while (pool->workers < pool->workers_min)
    reactor_pool_grow(pool);
}

void reactor_pool_enqueue(reactor_pool *pool, reactor_user_callback *callback, void *state)
{
  reactor_pool_job *job;
  ssize_t n;

  job = malloc(sizeof *job);
  reactor_user_construct(&job->user, callback, state);
  if (pool->jobs >= pool->workers)
    reactor_pool_grow(pool);
  if (!pool->jobs)
    reactor_core_fd_register(pool->queue[0], reactor_pool_event, pool, REACTOR_CORE_FD_MASK_READ);

  pool->jobs ++;

  if (TAILQ_EMPTY(&pool->jobs_head))
    {
      n = write(pool->queue[0], &job, sizeof job);
      if (n == sizeof job)
        return;
      reactor_core_fd_set(pool->queue[0], REACTOR_CORE_FD_MASK_WRITE);
    }
  TAILQ_INSERT_TAIL(&pool->jobs_head, job, entries);
}
