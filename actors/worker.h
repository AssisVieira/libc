#ifndef WORKER_H
#define WORKER_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

#include "queue/queue.h"

#define WORKER_QUEUE_MAX_SIZE 4096

typedef struct Worker Worker;

/**
 * Handle the messages sent to the worker.
 *
 * @param context  mutable local storage of worker.
 * @param msg      message to be handled.
 * @return         true, if you want to continue processing the messages,
 *                 otherwise, false. In this second case, the worker will be
 *                 closed.
 */
typedef bool (*WorkerHandler)(void *context, const void *msg);

typedef struct Worker {
  Queue *msgs;
  WorkerHandler handler;
  thrd_t thread;
  atomic_bool close;
  void *context;
} Worker;

Worker *worker_create(const char *name, WorkerHandler handler,
                      size_t contextSize);

void worker_send(Worker *worker, void *msg, size_t size);

void worker_await(Worker *worker);

#endif