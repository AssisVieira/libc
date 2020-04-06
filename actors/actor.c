#include "actor.h"

static bool actor_handler(void *context, const void *msg);

Actor *actor_create(Actor *parent, const ActorSpec *spec) {
  Actor *actor = malloc(sizeof(Actor));
  actor->parent = parent;
  actor->numChildren = 0;
  actor->context = actor_spec_context_create(spec);
  actor->spec = spec;
  actor->worker = worker_create(spec->name, actor_handler, sizeof(Actor));
  return actor;
}

static Msg *actor_on_closed(Msg *msg) {
  //
  actor_destroy(msg->from);
  //
}

void actor_close(Actor *actor) {
  Msg *msg = msg_create(NULL, &Close, NULL);
  worker_send(actor->worker, msg, sizeof(Msg));
}

static void actor_destroy(Actor *actor) {
  actor_spec_context_destroy(actor->context);
  free(actor);
}

static bool actor_handler(void *context, const void *arg) {
  Actor *actor = context;
  const Msg *msg = arg;

  Msg *resp = actor_spec_dispach(actor->spec, actor->context, msg);

  if (resp != NULL) {
    actor_send(resp->to, resp->from, msg->meta, msg->params);
  }

  return true;
}
