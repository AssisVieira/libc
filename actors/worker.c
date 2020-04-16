#include "worker.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

const WorkerSignal Close = 1;

static int worker_loop(void *arg);
static void worker_destroy(Worker *worker);

static void worker_state_init(Worker *worker);
static void worker_state_receive(Worker *worker);
static void worker_state_close(Worker *worker);

Worker *worker_create(const char *name, WorkerHandler handler,
                      size_t contextSize) {
  Worker *worker = malloc(sizeof(Worker));
  worker->close = false;
  worker->handler = handler;
  worker->context = malloc(contextSize);
  worker->msgs = queue_create(WORKER_QUEUE_MAX_SIZE);
  worker->state = worker_state_init;

  thrd_create(&worker->thread, worker_loop, worker);
 
  pthread_setname_np(worker->thread, name);

  return worker;
}

void worker_send(Worker *worker, void *msg, size_t size) {
  void *cloneMsg = NULL;

  if (size > 0) {
    cloneMsg = malloc(size);
    memcpy(cloneMsg, msg, size);
  } else {
    cloneMsg = msg;
  }

  queue_add(worker->msgs, cloneMsg, true);
}

static void worker_state_init(Worker *worker) {
  worker->state = worker->handle(worker->context, &Init);
}

static void worker_state_receive(Worker *worker) {
  void *msg = queue_get(worker->msgs, true);
  worker->state = worker->handle(worker->context, msg);
  free(msg);
}

static void worker_state_close(Worker *worker) {
  worker->close = true;
  worker->handler(worker->context, &Close);
}

static int worker_loop(void *arg) {
  Worker *worker = arg;

  while (!worker->close) {
    worker->state(worker);
  }

  worker_destroy(worker);

  return 0;
}

void worker_await(Worker *worker) { thrd_join(worker->thread, NULL); }

static void worker_destroy(Worker *worker) {
  while (worker->msgs != NULL) {
    void *msg = queue_get(worker->msgs, false);
    if (msg != NULL) {
      free(msg);
    } else {
      queue_destroy(worker->msgs);
      worker->msgs = NULL;
    }
  }

  free(worker->context);
  free(worker);
}
