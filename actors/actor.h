////////////////////////////////////////////////////////////////////////////////
// Actor
////////////////////////////////////////////////////////////////////////////////

#ifndef ACTOR_INCLUDE_H
#define ACTOR_INCLUDE_H

#include "foundation/foundation.h"

typedef struct ActorCell ActorCell;
typedef struct Msg Msg;

typedef void (*ActorOnStart)(ActorCell *actor, Msg *msg);
typedef void (*ActorOnReceive)(ActorCell *actor, Msg *msg);
typedef void (*ActorOnStop)(ActorCell *actor, Msg *msg);

typedef struct Actor {
  char *name;
  size_t stateSize;
  size_t paramsSize;
  ActorOnStart onStart;
  ActorOnReceive onReceive;
  ActorOnStop onStop;
} Actor;

/**
 * Declara um ator e uma struct usada pra inicializa-lo.
 *
 * @params typeName   nome do tipo do ator.
 * @params paramsName nome da struct de inicialização.
 */
#define ACTOR(name, params)                            \
  typedef struct name##Params params name##Params;     \
  extern const Actor name


/**
 * Define a implementação do ator e a estrutura do contexto do ator.
 */
#define ACTOR_IMPL(typeName, state) \
  static void typeName##_on_start(ActorCell *actor, Msg *msg);        \
  static void typeName##_on_receive(ActorCell *actor, Msg *msg);      \
  static void typeName##_on_stop(ActorCell *actor, Msg *msg);         \
  typedef struct typeName##State state typeName##State;               \
  const Actor typeName = {                                            \
      .name = #typeName,                                              \
      .stateSize = sizeof(typeName##State),                           \
      .paramsSize = sizeof(typeName##Params),                         \
      .onStart = typeName##_on_start,                                 \
      .onReceive = typeName##_on_receive,                             \
      .onStop = typeName##_on_stop,                                   \
  }


#define ACTOR_ON_START(typeName, pActor, pParams, pState, pMsg) \
  inline static void typeName##_on_start_intern(ActorCell *, typeName##Params *, typeName##State *, const Msg *); \
  static void typeName##_on_start(ActorCell *actor, Msg *msg) { \
    typeName##_on_start_intern(actor, actor->params, actor->state, msg); \
  } \
  inline static void typeName##_on_start_intern(ActorCell *pActor, typeName##Params *pParams, typeName##State *pState, const Msg *pMsg)


#define ACTOR_ON_RECEIVE(typeName, pActor, pParams, pState, pMsg) \
  inline static void typeName##_on_receive_intern(ActorCell *, typeName##Params *, typeName##State *, const Msg *); \
  static void typeName##_on_receive(ActorCell *actor, Msg *msg) { \
    typeName##_on_receive_intern(actor, actor->params, actor->state, msg); \
  } \
  inline static void typeName##_on_receive_intern(ActorCell *pActor, typeName##Params *pParams, typeName##State *pState, const Msg *pMsg)


#define ACTOR_ON_STOP(typeName, pActor, pParams, pState, pMsg) \
  inline static void typeName##_on_stop_intern(ActorCell *, typeName##Params *, typeName##State *, const Msg *); \
  static void typeName##_on_stop(ActorCell *actor, Msg *msg) { \
    typeName##_on_stop_intern(actor, actor->params, actor->state, msg); \
  } \
  inline static void typeName##_on_stop_intern(ActorCell *pActor, typeName##Params *pParams, typeName##State *pState, const Msg *pMsg)


#endif
