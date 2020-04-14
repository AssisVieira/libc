#include "actor.h"

static bool actor_handler(void *context, const void *msg);
static void actor_send_internal(Actor *from, Actor *to, const MsgType *type,
                                const void *params, size_t paramsSize);

Actor *actor_create(Actor *parent, const ActorType *type, const char *name,
                    size_t contextSize, const void *initParams,
                    size_t initParamsSize, ActorDispacher dispacher) {
  Actor *actor = malloc(sizeof(Actor));
  actor->parent = parent;
  actor->numChildren = 0;
  actor->context = malloc(contextSize);
  actor->type = type;
  actor->dispacher = dispacher;
  actor->worker = worker_create(name, actor_handler, sizeof(Actor));

  actor_send_internal(actor->parent, actor, &Init, initParams, initParamsSize);

  return actor;
}

void actor_await(Actor *actor) {
  worker_await(actor->worker);
}

void actor_close(Actor *actor) {
  actor_send(actor->parent, actor, &Close, NULL);
}

void actor_closeChildren(Actor *actor) {
	for (int i = 0; i < actor->numChildren; i++) {
	  actor_send(me, actor->children[i], &Close, NULL);
	}
}

static void actor_destroy(Actor *parent, int childId) {
  Actor *child = &parent->children[childId];
  actor_await(actor);
  free(child->context);
  free(child);
}

static void actor_send_internal(Actor *from, Actor *to, const MsgType *type,
                                const void *params, size_t paramsSize) {
  Msg *msg = msg_create(from, type, params, paramsSize);
  worker_send(to->worker, msg, 0);
}

Msg *actor_reply(const MsgType *type, const void *params) {
  return msg_create(NULL, type, params, type->paramsSize);
}

void actor_send(Actor *from, Actor *to, const MsgType *type,
                const void *params) {
  actor_send_internal(from, to, type, params, type->paramsSize);
}

static bool actor_handler(void *context, const void *arg) {
  Actor *actor = context;
  const Msg *msg = arg;

  ActorState state = actor->dispacher(actor->type, actor->context, msg);

  if (resp != NULL) {
    actor_send(actor, msg->from, resp->type, resp->params);
  }

  return (resp == NULL || resp->type->id != Closed.id);
}

