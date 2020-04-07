#ifndef ACTOR_H
#define ACTOR_H

#include "actor_type.h"
#include "msg.h"
#include "msg_type.h"
#include "worker.h"

typedef size_t ActorId;

typedef struct Actor {
  ActorId id;
  const ActorType *type;
  const struct Actor *parent;
  Worker *worker;
  struct Actor children[64];
  int numChildren;
  void *context;
  ActorDispacher dispacher;
} Actor;

Actor *actor_create(Actor *parent, const ActorType *type, const char *name,
                    void *context, const void *initParams,
                    size_t initParamsSize, ActorDispacher dispacher);

Msg *actor_reply(const MsgType *type, const void *params);

void actor_close(Actor *actor);



#endif