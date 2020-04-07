#include "actor_type.h"

#include <stdatomic.h>

static void actor_type_register_all_msg_type(ActorType *type);
static Msg *actor_type_handler_default(const Msg *msg);
static ActorHandlers actor_type_create_handlers(const ActorType *type);
static Msg *actor_type_dispach(ActorType *type, void *context, const Msg *msg);

////////////////////////////////////////////////////////////////////////////////

Actor *actor_type_create_actor(Actor *parent, const ActorType *type,
                               const void *initParams) {
  void *context = malloc(type->contextSize);

  actor_type_register(type);

  return actor_create(parent, type, type->name, context, initParams,
                      type->initParamsSize, actor_type_dispach);
}

////////////////////////////////////////////////////////////////////////////////

static void actor_type_register(const ActorType *type) {
  ActorType *typePtr = (ActorType *)typePtr;

  if (typePtr->registred) return;

  mtx_lock(&typePtr->lock);
  if (!typePtr->registred) {
    actor_type_register_all_msg_type(typePtr);
    typePtr->handlers = actor_type_create_handlers(typePtr);
    typePtr->registred = true;
  }
  mtx_unlock(&typePtr->lock);
}

////////////////////////////////////////////////////////////////////////////////

static void actor_type_register_all_msg_type(ActorType *type) {
  for (int i = 0; type->routes[i].type != NULL; i++) {
    const MsgType *msgType = &type->routes[i];
    msg_type_register(msgType);
  }
}

////////////////////////////////////////////////////////////////////////////////

static size_t actor_type_maxMsgTypeId(const ActorType *type) {
  int max = 0;
  for (int i = 0; type->routes[i].type != NULL; i++) {
    Route *route = &type->routes[i];
    if (max < route->type->id) max = route->type->id;
  }
  return max;
}

////////////////////////////////////////////////////////////////////////////////

static Msg *actor_type_handler_default(const Msg *msg) { return NULL; }

////////////////////////////////////////////////////////////////////////////////

static Msg *actor_type_on_done(const Msg *msg) {
  return actor_reply(&Close, NULL);
}

////////////////////////////////////////////////////////////////////////////////

static Msg *actor_type_on_close(const Msg *msg) {
  return actor_reply(&Closed, NULL);
}

////////////////////////////////////////////////////////////////////////////////

static Msg *actor_type_on_closed(const Msg *msg) {
  actor_destroy(msg->from);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static ActorHandlers actor_type_create_handlers(ActorType *type) {
  size_t maxMsgTypeId = 0;
  ActorHandlers handlers = NULL;

  handlers[Done.id] = actor_type_on_done;
  handlers[Close.id] = actor_type_on_close;
  handlers[Closed.id] = actor_type_on_closed;

  maxMsgTypeId = actor_type_maxMsgTypeId(type);

  handlers = malloc(sizeof(ActorHandler) * maxMsgTypeId);

  for (int i = 0; i < maxMsgTypeId; i++) {
    handlers[i] = actor_type_handler_default;
  }

  for (int i = 0; type->routes[i].type != NULL; i++) {
    Route *route = &type->routes[i];
    handlers[route->type->id] = route->handler;
  }

  return handlers;
}

////////////////////////////////////////////////////////////////////////////////

static Msg *actor_type_dispach(ActorType *type, void *context, const Msg *msg) {
  return type->handlers[msg->type->id](context, msg->params);
}
