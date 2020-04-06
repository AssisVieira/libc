#ifndef ACTOR_H
#define ACTOR_H

#include "actor_spec.h"
#include "msg.h"
#include "msg_spec.h"
#include "worker.h"

typedef size_t ActorId;

typedef struct Actor {
  ActorId id;
  Worker *worker;
  const struct Actor *parent;
  struct Actor children[64];
  int numChildren;
  const ActorSpec *spec;
  void *context;
} Actor;

Actor *actor_create(Actor *parent, const ActorSpec *spec);

void actor_close(Actor *actor);

MESSAGE(Close, {});

#endif