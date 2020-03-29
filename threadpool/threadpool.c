#include "threadpool.h"

#include <threads.h>

#include "queue/queue.h"

////////////////////////////////////////////////////////////////////////////////
/// WORKER /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct Worker {
  thrd_t thread;
  Queue *jobs;
  struct Worker **workers;
  int numWorkers;
  volatile bool close;
} Worker;

/**
 * Create a worker.
 *
 * @param maxJobs maximum number of jobs in the queue.
 */
static Worker *worker_create(int maxJobs, Worker **workers, int numWorkers);

/**
 * Handler to be executed by the thread.
 *
 * @param arg  worker.
 */
static int worker_execute(void *arg);

/**
 * Add a task to the queue of the worker.
 */
static bool worker_add(Worker *worker, void (*task)(void *), void *arg);

/**
 * Asynchronously, closes the worker, interrupts the consumption of the job
 * queue and destroys the worker.
 */
static void worker_close(Worker *worker);

/**
 * Waits the worker be closed.
 */
static void worker_join(Worker *worker);

/**
 * Destroy the worker.
 */
static void worker_destroy(Worker *worker);

////////////////////////////////////////////////////////////////////////////////

static Worker *worker_create(int maxJobs, Worker **workers, int numWorkers) {
  Worker *worker = malloc(sizeof(Worker));
  worker->jobs = queue_create(maxJobs);
  worker->workers = workers;
  worker->numWorkers = numWorkers;
  worker->close = false;
  thrd_create(&worker->thread, worker_execute, worker);
  return worker;
}

////////////////////////////////////////////////////////////////////////////////

static bool worker_add(Worker *worker, Actor *actor) {
  return queue_add(worker->jobs, actor);
}

////////////////////////////////////////////////////////////////////////////////

static int worker_execute(void *arg) {
  Worker *worker = arg;
  Actor *actor = NULL;
  Message *msg = NULL;

  while (!worker->close) {
    while (!worker->close) {
      actor = queue_get(worker->jobs);

      if (actor == NULL) {
        int iSibling = rand() % worker->numWorkers;
        actor = queue_get(worker->workers[iSibling]->jobs);
      }

      if (actor != NULL) break;

      sleep(1);
    }

    while (!worker->close && (msg = queue_get(actor->messages)) != NULL) {
      actor->handler(actor->context, msg->type, msg->content, msg->size);
      message_destroy(msg);
    }

    actor->scheduled = false;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

// static void worker_close(Worker *worker) { worker->close = true; }

////////////////////////////////////////////////////////////////////////////////

static void worker_join(Worker *worker) { thrd_join(worker->thread, NULL); }

////////////////////////////////////////////////////////////////////////////////

static void worker_destroy(Worker *worker) {
  queue_destroy(worker->jobs);
  free(worker);
}