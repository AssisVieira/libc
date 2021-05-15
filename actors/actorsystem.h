#ifndef ACTORSYSTEM_INCLUDE_H
#define ACTORSYSTEM_INCLUDE_H

#include "actor.h"
#include "foundation/foundation.h"

typedef struct Dispatcher Dispatcher;
typedef struct Msg Msg;
typedef struct ActorCell ActorCell;
typedef struct MailBox MailBox;
typedef struct MsgType MsgType;
typedef struct Actor Actor;

typedef struct ActorSystem {
  Dispatcher *dispatcher;
  atomic_bool stop;
  atomic_bool stopChildren;
  atomic_bool stopChildrenDone;
  pthread_cond_t waitCond;
  pthread_mutex_t waitMutex;
} ActorSystem;

ACTOR(System, { char ignore; });

ActorCell *actors_child_new(ActorCell *parent, const char *name,
                            const Actor *actor, const void *params);
ActorCell *actors_create(int cores);
void actors_send(ActorCell *from, ActorCell *to, const MsgType *type,
                 const void *payload);
int actors_wait_children(ActorCell *system);
void actors_free(ActorCell *system);
int actors_num_children(ActorCell *actor);

void actors_stop_self(ActorCell *actor);

#endif
