////////////////////////////////////////////////////////////////////////////////
// Worker
////////////////////////////////////////////////////////////////////////////////

#ifndef WORKER_INCLUDE_H
#define WORKER_INCLUDE_H

#include "foundation/foundation.h"

typedef struct Dispatcher Dispatcher;
typedef struct ActorCell ActorCell;

typedef struct Worker {
  pthread_t thread;
  Queue *queue;
  atomic_bool stop;
  pthread_cond_t condNotEmpty;
  pthread_mutex_t mutex;
  int throughput;
  int throughputDeadlineNS;
  Dispatcher *dispatcher;
  int core;
  char *name;
} Worker;

int worker_set_core_affinity(int core);
Worker *worker_create(Dispatcher *dispatcher, const char *name, int core, int throughput, int throughputDeadlineNS);
void worker_free(Worker *worker);
void worker_enqueue(Worker *worker, ActorCell *actor);
long worker_current_time_ns();
void *worker_run(void *arg);
void worker_stop(Worker *worker);

#endif
