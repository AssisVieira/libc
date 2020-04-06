#include "actor_spec.h"

#include <stdatomic.h>

static void actor_spec_register(const ActorSpec *meta) {
  ActorSpec *me = (ActorSpec *)meta;

  if (me->registred) return;

  mtx_lock(&me->lock);

  if (!me->registred) {
    for (int i = 0; me->routes[i].metaMsg != NULL; i++) {
      const MetaMsg *metaMsg = &me->routes[i];
      meta_msg_register(metaMsg);
    }
    me->registred = true;
  }

  mtx_unlock(&me->lock);
}

static size_t actor_spec_maxMetaMsgId(const ActorSpec *meta) {
  int max = 0;
  for (int i = 0; meta->routes[i].metaMsg != NULL; i++) {
    Route *route = &meta->routes[i];
    if (max < route->metaMsg->id) max = route->metaMsg->id;
  }
  return max;
}

static Msg *actor_handler_default(const Msg *msg) { return NULL; }

Handlers actor_spec_handlers_create(const ActorSpec *meta) {
  size_t maxMetaMsgId = 0;
  Handlers handlers = NULL;

  actor_spec_register(meta);

  maxMetaMsgId = actor_spec_maxMetaMsgId(meta);

  handlers = malloc(sizeof(Handler) * maxMetaMsgId);

  for (int i = 0; i < maxMetaMsgId; i++) {
    handlers[i] = actor_handler_default;
  }

  for (int i = 0; meta->routes[i].metaMsg != NULL; i++) {
    Route *route = &meta->routes[i];
    handlers[route->metaMsg->id] = route->handler;
  }

  return handlers;
}

Msg *actor_spec_dispach(const ActorSpec *spec, const Msg *msg) { return NULL; }