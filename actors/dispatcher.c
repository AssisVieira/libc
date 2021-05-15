////////////////////////////////////////////////////////////////////////////////
// Dispatcher
////////////////////////////////////////////////////////////////////////////////

#include "dispatcher.h"

#include "actorcell.h"
#include "msg.h"
#include "worker.h"

Dispatcher *dispatcher_create(int numWorkers) {
  Dispatcher *dispatcher = malloc(sizeof(Dispatcher));
  dispatcher->workers = malloc(sizeof(Worker *) * numWorkers);
  dispatcher->numWorkers = numWorkers;
  dispatcher->currentWorker = 0;

  int throughput = 8;
  int throughputDeadlineNS = -1;  // not defined

  int numCores = sysconf(_SC_NPROCESSORS_ONLN);

  for (int i = 0; i < numWorkers; i++) {
    char name[32] = {0};
    snprintf(name, sizeof(name) - 1, "worker-%d", i);
    name[31] = '\0';
    dispatcher->workers[i] = worker_create(dispatcher, name, i % numCores,
                                           throughput, throughputDeadlineNS);
  }

  return dispatcher;
}

void dispatcher_free(Dispatcher *dispatcher) {
  for (int i = 0; i < dispatcher->numWorkers; i++) {
    worker_stop(dispatcher->workers[i]);
  }
  for (int i = 0; i < dispatcher->numWorkers; i++) {
    worker_free(dispatcher->workers[i]);
  }
  free(dispatcher->workers);
  free(dispatcher);
}

void dispatcher_dispatch(Dispatcher *dispatcher, ActorCell *actor) {
  if (actorcell_set_scheduled(actor)) {
    dispatcher_execute(dispatcher, actor);
  }
}

void dispatcher_execute(Dispatcher *dispatcher, ActorCell *actor) {
  int worker = actorcell_worker(actor);
  bool undefinedWorker = (worker < 0) ? true : false;
  bool affinity = actorcell_affinity(actor);

  if (!affinity || (affinity && undefinedWorker)) {
    int currentWorker = dispatcher->currentWorker;
    int nextWorker = (currentWorker + 1) % dispatcher->numWorkers;

    while (!atomic_compare_exchange_weak(&dispatcher->currentWorker,
                                         &currentWorker, nextWorker)) {
      nextWorker = (currentWorker + 1) % dispatcher->numWorkers;
    }

    worker = nextWorker;
  }

  actorcell_set_worker(actor, worker);

  worker_enqueue(dispatcher->workers[worker], actor);
}
