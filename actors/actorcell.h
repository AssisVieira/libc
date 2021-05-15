#ifndef ACTORCELL_INCLUDE_H
#define ACTORCELL_INCLUDE_H

#include "actor.h"
#include "foundation/foundation.h"

typedef struct MsgType MsgType;
typedef struct Msg Msg;
typedef struct MsgType MsgType;
typedef struct Dispatcher Dispatcher;
typedef struct MailBox MailBox;
typedef struct ActorRef ActorRef;
typedef struct ActorCell ActorCell;

typedef struct ActorCellList {
  ActorCell *actor;
  struct ActorCellList *next;
} ActorCellList;

typedef struct ActorCell {
  MailBox *mailbox;
  Actor actor;
  void *params;
  void *state;
  Dispatcher *dispatcher;
  ActorCell *parent;
  ActorCellList *children;
  atomic_int childrenCount;
  bool stopping;
  char *name;
  atomic_bool idle;
  bool affinity;
  int worker;
  bool stopSelf;
} ActorCell;

int actorcell_add_child(ActorCell *parent, ActorCell *child);
bool actorcell_remove_child(ActorCell *parent, ActorCell *child);
int actorcell_children_count(const ActorCell *actor);
ActorCell *actorcell_create(ActorCell *parent, const char *name,
                            const Actor *actor, const void *params,
                            Dispatcher *dispatcher);
void actorcell_free(ActorCell *actorCell);
void actorcell_start(ActorCell *actorCell);
void actorcell_send(ActorCell *from, ActorCell *to, const MsgType *type,
                    const void *payload);
void actorcell_stop(ActorCell *actorCell, Msg *msg);
void actorcell_stopped(ActorCell *actorCell, Msg *msg);
bool actorcell_receive(ActorCell *actorCell, Msg *msg);
const char *actorcell_name(const ActorCell *actorCell);

bool actorcell_is_empty(const ActorCell *actorCell);
bool actorcell_is_idle(const ActorCell *actorCell);
bool actorcell_set_scheduled(ActorCell *actorCell);
bool actorcell_set_idle(ActorCell *actorCell);

bool actorcell_affinity(const ActorCell *actorCell);

int actorcell_worker(const ActorCell *actorCell);

void actorcell_set_worker(ActorCell *actorCell, int worker);

bool actorcell_process(ActorCell *actorCell);

int actorcell_num_children(ActorCell *actorCell);

void actorcell_stop_self(ActorCell *actor);

#endif
