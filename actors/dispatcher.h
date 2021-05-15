#ifndef DISPATCHER_INCLUDE_H
#define DISPATCHER_INCLUDE_H

#include "foundation/foundation.h"

typedef struct Worker Worker;
typedef struct ActorCell ActorCell;

typedef struct Dispatcher {
  Worker **workers;
  int numWorkers;
  atomic_int currentWorker;
} Dispatcher;

Dispatcher *dispatcher_create(int numWorkers);
void dispatcher_free(Dispatcher *dispatcher);
void dispatcher_dispatch(Dispatcher *dispatcher, ActorCell *actor);
void dispatcher_execute(Dispatcher *dispatcher, ActorCell *actor);

#endif
