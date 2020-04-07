#ifndef ACTOR_TYPE_H
#define ACTOR_TYPE_H

#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

#include "msg.h"
#include "msg_type.h"

#define ACTOR_TYPE_ROUTES_MAX 128

typedef Msg *(*ActorHandler)(void *context, const void *params);

typedef Msg *(*ActorDispacher)(ActorType *type, void *context, const Msg *msg);

typedef ActorHandler *ActorHandlers;

typedef struct Route {
  const MsgType *const type;
  ActorHandler handler;
} Route;

typedef struct ActorType {
  char *name;
  size_t contextSize;
  size_t initParamsSize;
  Route routes[128];
  ActorHandlers handlers;
  bool registred;
  mtx_t lock;
} ActorType;

Actor *actor_type_create_actor(Actor *parent, const ActorType *type,
                               const void *initParams);

/**
 * Declara um ator e uma struct usada pra inicializa-lo.
 *
 * @params typeName   nome do tipo de ator.
 * @params paramsName nome da struct de inicialização.
 * @params varargs    bloco de expressão com os campos da struct.
 */
#define ACTOR(typeName, initParams)                                    \
  typedef struct typeName##InitParams initParams typeName##InitParams; \
  extern const ActorType typeName;

/**
 * Define a implementação do ator e os parâmetros do contexto do ator.
 */
#define ACTOR_IMPL(typeName, context, ...)                    \
  typedef struct typeName##Context context typeName##Context; \
  const ActorType typeName = {                                \
      .name = #typeName,                                      \
      .contextSize = sizeof(typeName##Context),               \
      .initParamsSize = sizeof(typeName##InitParams),         \
      .routes = __VA_ARGS__,                                  \
      .handlers = NULL,                                       \
      .registred = false,                                     \
  }

MESSAGE(Init, {});
MESSAGE(Close, {});
MESSAGE(Closed, {});
MESSAGE(Done, {});

#endif