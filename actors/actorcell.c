////////////////////////////////////////////////////////////////////////////////
// ActorCell
////////////////////////////////////////////////////////////////////////////////

#include "actorcell.h"

#include "dispatcher.h"
#include "mailbox.h"
#include "msg.h"

int actorcell_add_child(ActorCell *parent, ActorCell *child) {
  ActorCellList *node = malloc(sizeof(ActorCellList));

  node->next = parent->children;
  node->actor = child;
  parent->children = node;

  int count = ++parent->childrenCount;

  return count;
}

bool actorcell_remove_child(ActorCell *parent, ActorCell *child) {
  bool removed = false;

  ActorCellList *node = parent->children;
  ActorCellList *prev = NULL;
  while (node != NULL) {
    if (node->actor == child) {
      if (prev == NULL) {
        parent->children = node->next;
      } else {
        prev->next = node->next;
      }
      free(node);
      removed = true;
      break;
    }
    prev = node;
    node = node->next;
  }

  --parent->childrenCount;

  return removed;
}

int actorcell_children_count(const ActorCell *actor) {
  return actor->childrenCount;
}

void actorcell_stop_self(ActorCell *actor) {
  actor->stopSelf = true;
}

ActorCell *actorcell_create(ActorCell *parent, const char *name,
                            const Actor *actor, const void *params,
                            Dispatcher *dispatcher) {
  ActorCell *actorCell = malloc(sizeof(ActorCell));
  actorCell->actor = *actor;
  actorCell->idle = true;
  actorCell->affinity = true;
  actorCell->worker = -1;
  actorCell->mailbox = mailbox_create(1000);
  actorCell->dispatcher = dispatcher;
  actorCell->state = calloc(1, actor->stateSize);
  actorCell->parent = parent;
  actorCell->children = NULL;
  actorCell->childrenCount = 0;
  actorCell->stopping = false;
  actorCell->name = strdup(name);
  actorCell->stopSelf = false;

  actorCell->params = calloc(1, actor->paramsSize);

  if (params != NULL) {
    memcpy(actorCell->params, params, actor->paramsSize);
  }

  if (parent != NULL) {
    actorcell_add_child(parent, actorCell);
  }

  actorcell_send(parent, actorCell, &Start, NULL);

  return actorCell;
}

bool actorcell_is_empty(const ActorCell *actorCell) {
  return mailbox_is_empty(actorCell->mailbox);
}

bool actorcell_is_idle(const ActorCell *actorCell) { return actorCell->idle; }

bool actorcell_set_scheduled(ActorCell *actorCell) {
  bool expected = true;
  return atomic_compare_exchange_strong(&actorCell->idle, &expected, false);
}

bool actorcell_set_idle(ActorCell *actorCell) {
  bool expected = false;
  return atomic_compare_exchange_strong(&actorCell->idle, &expected, true);
}

void actorcell_free(ActorCell *actorCell) {
  debugf("%s freeing...", actorCell->name);
  free(actorCell->state);
  free(actorCell->params);
  mailbox_free(actorCell->mailbox);
  free(actorCell->name);
  free(actorCell->children);
  free(actorCell);
}

int actorcell_worker(const ActorCell *actorCell) { return actorCell->worker; }

void actorcell_set_worker(ActorCell *actorCell, int worker) {
  actorCell->worker = worker;
}

bool actorcell_affinity(const ActorCell *actorCell) {
  return actorCell->affinity;
}

void actorcell_send(ActorCell *from, ActorCell *to, const MsgType *type,
                    const void *payload) {
  debugf("[send] %s >>> %s >>> %s",
         (from != NULL) ? from->name : "[main-thread]", type->name, to->name);
  Msg *msg = msg_create(from, type, payload);
  mailbox_push(to->mailbox, msg);
  dispatcher_dispatch(to->dispatcher, to);
}

void actorcell_stop(ActorCell *actorCell, Msg *msg) {
  actorCell->stopping = true;

  ActorCellList *child = actorCell->children;

  while (child != NULL) {
    actorcell_send(actorCell, child->actor, &Stop, NULL);
    child = child->next;
  }
}

void actorcell_stopped(ActorCell *actorCell, Msg *msg) {
  ActorCell *child = msg->from;
  if (actorcell_remove_child(actorCell, child)) {
    actorcell_free(child);
  }
}

int actorcell_num_children(ActorCell *actorCell) {
  return actorCell->childrenCount;
}

bool actorcell_receive(ActorCell *actorCell, Msg *msg) {
  debugf("[receive] %s >>> %s >>> %s", (msg->from) ? msg->from->name : "[main-thread]", msg->type->name, actorCell->name);

  if (msg->type == &Start) {
    actorCell->actor.onStart(actorCell, msg);
    return true;
  }

  if (msg->type == &Stop) {
    actorcell_stop(actorCell, msg);
  } else {
    actorCell->actor.onReceive(actorCell, msg);

    if (msg->type == &Stopped) {
      actorcell_stopped(actorCell, msg);
    }

    if (actorCell->stopSelf) {
      actorcell_stop(actorCell, NULL);
    }
  }

  if (actorCell->stopping && actorCell->children == NULL) {
    actorCell->actor.onStop(actorCell, msg);
    if (actorCell->parent != NULL) {
      actorcell_send(actorCell, actorCell->parent, &Stopped, NULL);
    }
    return false;
  }

  return true;
}

const char *actorcell_name(const ActorCell *actorCell) {
  return actorCell->name;
}

bool actorcell_process(ActorCell *actorCell) {
  bool keepGoing = true;

  Msg *msg = mailbox_pull(actorCell->mailbox);

  if (msg != NULL) {
    keepGoing = actorcell_receive(actorCell, msg);
    msg_free(msg);
  }

  return keepGoing;
}
