////////////////////////////////////////////////////////////////////////////////
// Worker
////////////////////////////////////////////////////////////////////////////////

#include "worker.h"

#include "actorcell.h"
#include "actorsystem.h"
#include "dispatcher.h"
#include "mailbox.h"

int worker_set_core_affinity(int core) {
  int numCores = sysconf(_SC_NPROCESSORS_ONLN);

  if (core < 0 || core >= numCores) return EINVAL;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);

  pthread_t currentThread = pthread_self();

  return pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuset);
}

Worker *worker_create(Dispatcher *dispatcher, const char *name, int core,
                      int throughput, int throughputDeadlineNS) {
  Worker *worker = malloc(sizeof(Worker));
  worker->dispatcher = dispatcher;
  worker->stop = false;
  worker->core = core;
  worker->queue = queue_create(1000);
  worker->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  worker->condNotEmpty = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
  worker->throughputDeadlineNS = throughputDeadlineNS;
  worker->throughput = throughput;
  worker->name = strdup(name);

  pthread_create(&worker->thread, NULL, worker_run, worker);

  pthread_setname_np(worker->thread, name);

  return worker;
}

void worker_free(Worker *worker) {
  pthread_join(worker->thread, NULL);

  queue_free(worker->queue);

  free(worker->name);

  free(worker);
}

void worker_enqueue(Worker *worker, ActorCell *actor) {
  pthread_mutex_lock(&worker->mutex);

  queue_add(worker->queue, actor);

  pthread_cond_signal(&worker->condNotEmpty);

  pthread_mutex_unlock(&worker->mutex);
}

long worker_current_time_ns() {
  long ns;
  time_t sec;
  struct timespec spec;
  const long billion = 1000000000L;

  clock_gettime(CLOCK_REALTIME, &spec);
  sec = spec.tv_sec;
  ns = spec.tv_nsec;

  return (uint64_t)sec * billion + (uint64_t)ns;
}

void *worker_run(void *arg) {
  Worker *worker = (Worker *)arg;

  if (worker_set_core_affinity(worker->core)) {
    debug("set core affinity fail.");
  }

  while (!worker->stop) {
    pthread_mutex_lock(&worker->mutex);

    ActorCell *actor = queue_get(worker->queue);

    if (actor == NULL) {
      pthread_cond_wait(&worker->condNotEmpty, &worker->mutex);
    }

    pthread_mutex_unlock(&worker->mutex);

    if (actor == NULL) continue;

    int leftThroughput = worker->throughput;
    long deadlineNS = worker_current_time_ns() + worker->throughputDeadlineNS;
    bool keepGoing = true;

    while (keepGoing && leftThroughput > 0 &&
           ((worker->throughputDeadlineNS <= 0) ||
            (worker_current_time_ns() - deadlineNS < 0))) {
      keepGoing = actorcell_process(actor);
      leftThroughput--;
    }

    if (keepGoing) {
      actorcell_set_idle(actor);
        
      if (!actorcell_is_empty(actor)) {
        dispatcher_dispatch(worker->dispatcher, actor);
      }
    }
  }

  return NULL;
}

void worker_stop(Worker *worker) {
  pthread_mutex_lock(&worker->mutex);
  worker->stop = true;
  pthread_cond_signal(&worker->condNotEmpty);
  pthread_mutex_unlock(&worker->mutex);
}
