#include "worker.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static int worker_loop(void *arg);
static void worker_destroy(Worker *worker);

Worker *worker_create(const char *name, WorkerHandler handler,
                      size_t contextSize) {
  Worker *worker = malloc(sizeof(Worker));
  worker->close = false;
  worker->handler = handler;
  worker->context = malloc(contextSize);
  worker->msgs = queue_create(WORKER_QUEUE_MAX_SIZE);

  thrd_create(&worker->thread, worker_loop, worker);
  pthread_setname_np(worker->thread, name);

  return worker;
}

void worker_send(Worker *worker, const void *msg, size_t size) {
  void *cloneMsg = malloc(size);
  memcpy(cloneMsg, msg, size);
  queue_add(worker->msgs, cloneMsg, true);
}

static int worker_loop(void *arg) {
  Worker *worker = arg;

  while (!worker->close) {
    void *msg = queue_get(worker->msgs, true);

    worker->close = !worker->handler(worker->context, msg);

    free(msg);
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
