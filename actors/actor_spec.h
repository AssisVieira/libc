#ifndef ACTOR_SPEC_H
#define ACTOR_SPEC_H

#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

#include "msg.h"
#include "msg_spec.h"

#define ACTOR_SPEC_ROUTES_MAX 128

typedef Msg *(*Handler)(void *context, const Msg *);

typedef Handler *Handlers;

typedef struct Route {
  const MsgSpec *const spec;
  Handler handler;
} Route;

typedef struct ActorSpec {
  char *name;
  size_t contextSize;
  Route routes[128];
  Handlers handlers;
  bool registred;
  mtx_t lock;
} ActorSpec;

Msg *actor_spec_dispach(const ActorSpec *spec, const Msg *msg);

/**
 * Declara um ator e uma struct usada pra inicializa-lo.
 *
 * @params actorName  nome do ator.
 * @params paramsName nome da struct de inicialização.
 * @params varargs    bloco de expressão com os campos da struct.
 */
#define ACTOR(actorName, initParams)                                     \
  typedef struct actorName##InitParams initParams actorName##InitParams; \
  extern const ActorSpec actorName;

/**
 * Define a implementação do ator e os parâmetros do contexto do ator.
 */
#define ACTOR_IMPL(actorName, context, ...)                     \
  typedef struct actorName##Context context actorName##Context; \
  const ActorSpec actorName = {                                 \
      .name = #actorName,                                       \
      .contextSize = sizeof(actorName##Context),                \
      .routes = __VA_ARGS__,                                    \
      .handlers = NULL,                                         \
      .registred = false,                                       \
  }

#endif