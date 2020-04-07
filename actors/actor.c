#include "actor.h"

static bool actor_handler(void *context, const void *msg);
static void actor_send_internal(Actor *from, Actor *to, const MsgType *type,
                                const void *params, size_t paramsSize);

Actor *actor_create(Actor *parent, const ActorType *type, const char *name,
                    void *context, const void *initParams,
                    size_t initParamsSize, ActorDispacher dispacher) {
  Actor *actor = malloc(sizeof(Actor));
  actor->parent = parent;
  actor->numChildren = 0;
  actor->context = context;
  actor->type = type;
  actor->dispacher = dispacher;
  actor->worker = worker_create(name, actor_handler, sizeof(Actor));

  actor_send_internal(actor->parent, actor, &Init, initParams, initParamsSize);

  return actor;
}

void actor_close(Actor *actor) {
  actor_send(actor->parent, actor, &Close, NULL);
}

static void actor_destroy(Actor *actor) {
  actor_spec_context_destroy(actor->context);
  free(actor);
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

  Msg *resp = actor->dispacher(actor->type, actor->context, msg);

  if (resp != NULL) {
    actor_send(actor, msg->from, resp->type, resp->params);
  }

  return true;
}
