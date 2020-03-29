

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "queue/queue.h"
#include "vetor/vetor.h"

////////////////////////////////////////////////////////////////////////////////
/// ACTOR MSG DEFINITIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct ActorMsg {
  void *context;
  Actor *me;
  Actor *to;
  ActorMsg type;
  int size;
  char params[];
} ActorMsg;

////////////////////////////////////////////////////////////////////////////////

ActorMsg *actor_msg_create(Actor *from, Actor *to, int type,
                           const void *params);

////////////////////////////////////////////////////////////////////////////////

Actor *actor_msg_to(const ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////

Actor *actor_msg_from(const ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////

int actor_msg_type(const ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////

int actor_msg_size(const ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////

const void *actor_msg_params(const ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////

void actor_msg_destroy(ActorMsg *msg);

////////////////////////////////////////////////////////////////////////////////
/// ACTOR DEFINITIONS //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define ACTOR_QUEUE_SIZE 32000

////////////////////////////////////////////////////////////////////////////////

typedef struct Actor {
  int id;
  char *name;
  Queue *msgs;
  thrd_t thread;
  void *context;
  struct Actor *parent;
  Vetor *children;
  Vetor *routes;
} Actor;

////////////////////////////////////////////////////////////////////////////////

typedef ActorMsg *(*ActorHandler)(const ActorMsg *);

////////////////////////////////////////////////////////////////////////////////

typedef int ActorMsgType;

////////////////////////////////////////////////////////////////////////////////

typedef struct ActorRoute {
  ActorMsgType *type;
  ActorHandler handler;
  int paramsSize;
} ActorRoute;

////////////////////////////////////////////////////////////////////////////////

typedef struct ActorMsgInfo {
  ActorMsgType *type;
  Actor
} ActorMsgInfo;

////////////////////////////////////////////////////////////////////////////////

typedef struct ActorInfo {
  char *name;
  int contextSize;
  ActorMsgInfo *msgs[128];
  ActorRoute routes[128];
} ActorInfo;

////////////////////////////////////////////////////////////////////////////////

ActorMsgType MSG_CREATE = 0;
ActorMsgType MSG_DESTROY = 0;
ActorMsgType MSG_KILLME = 0;

////////////////////////////////////////////////////////////////////////////////
/// ACTOR MSG IMPLEMENTATION ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ActorMsg *actor_msg_create(Actor *me, Actor *to, int type, const void *params) {
  ActorRoute *route = vetor_item(to->routes, type);
  ActorMsg *msg = malloc(sizeof(ActorMsg) + route->paramsSize);
  msg->me = me;
  msg->to = to;
  msg->context = to->context;
  msg->type = type;
  msg->size = route->paramsSize;
  memcpy(msg->params, params, route->paramsSize);
  return msg;
}

////////////////////////////////////////////////////////////////////////////////

void actor_msg_destroy(ActorMsg *message) { free(message); }

////////////////////////////////////////////////////////////////////////////////
/// ACTOR IMPLEMENTATION ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static int actor_worker(void *arg) {
  // Actor *me = arg;
  // Actor *parent = me->parent;
  // ActorMsg *msg = NULL;

  // typeset_t mask;
  // typefillset(&mask);
  // if (pthread_typemask(MSG_SETMASK, &mask, NULL)) {
  //   fprintf(stderr, "pthread_typemask()\n");
  //   return -1;
  // }

  // while (true) {
  //   while ((msg = queue_get(me->msgs)) == NULL) {
  //     thrd_yield();
  //   }

  //   if (msg->type == MSG_DESTROY) {
  //     for (int i = 0; i < vetor_qtd(me->children); i++) {
  //       Actor *child = vetor_item(me->children, i);
  //       if (child == NULL) continue;
  //       actor_destroy(me, i);
  //     }
  //   }

  //   if (msg->type == MSG_KILLME) {
  //     actor_destroy(me, msg->from->id);
  //   }

  //   ActorMsg *resp = me->receive(msg);

  //   if (resp != NULL) {
  //     if (msg->type == MSG_CREATE) {
  //       me->context = malloc(resp->size);
  //       memcpy(me->context, resp->params, resp->size);
  //     }

  //     if (msg->from->msgs != NULL) {
  //       resp->from = me;
  //       resp->to = msg->from;
  //       queue_add(msg->from->msgs, resp);
  //     } else {
  //       actor_msg_destroy(resp);
  //     }
  //   }

  //   if (msg->type == MSG_DESTROY) {
  //     break;
  //   }

  //   actor_msg_destroy(msg);
  // }

  // do {
  //   // Destroys the last message: ACTOR_DESTROY ...
  //   actor_msg_destroy(msg);
  // } while ((msg = queue_get(me->msgs)) != NULL);

  // if (parent != NULL) {
  //   vetor_inserir(parent->children, me->id, NULL);
  // }

  // queue_destroy(me->msgs);
  // vetor_destruir(me->children);
  // free(me->name);
  // free(me->context);
  // free(me);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

Actor *actor_create(ActorMsg *msg, ActorInfo *actorInfo, void *params) {
  static int ACTOR_COUNT = 0;
  static int MSG_COUNT = 0;

  Actor *actor = malloc(sizeof(Actor));
  actor->id = __sync_fetch_and_add(&ACTOR_COUNT, 1);
  actor->parent = (msg == NULL) ? NULL : msg->me;
  actor->name = strdup(actorInfo->name);
  actor->msgs = queue_create(ACTOR_QUEUE_SIZE);
  actor->children = vetor_criar(32);
  actor->routes = vetor_criar(32);

  for (int i = 0; actorInfo->routes[i].type != NULL; i++) {
    // Initialize the msgs: two or more instances of the same actor cannot be
    // created at same time.
    if (*actorInfo->routes[i].type == 0) {
      *actorInfo->routes[i].type = __sync_add_and_fetch(&MSG_COUNT, 1);
    }

    ActorRoute *route = malloc(sizeof(ActorRoute));
    *route = actorInfo->routes[i];
    vetor_inserir(actor->routes, route->type, route);
  }

  thrd_create(&actor->thread, actor_worker, actor);

  return actor;
}

////////////////////////////////////////////////////////////////////////////////

void actor_send(Actor *me, Actor *to, ActorMsgType type, const void *params) {
  ActorMsg *msg = actor_msg_create(me, to, type, params);
  while (!queue_add(to->msgs, msg)) thrd_yield();
}

////////////////////////////////////////////////////////////////////////////////

int actor_init(ActorInfo *actorInfo, void *params) {
  Actor *actor = actor_create(NULL, actorInfo, params);
  actor_send(NULL, actor, MSG_CREATE, params);
}

////////////////////////////////////////////////////////////////////////////////
/// PING ACTOR /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct PingContext {
  int max;
  int count;
} PingContext;

typedef struct PingCreate {
  int max;
} PingCreate;

ActorMsgType MSG_PING = 0;

static ActorMsg *ping_on_ping(const ActorMsg *msg);
static ActorMsg *ping_on_create(const ActorMsg *msg);

const ActorInfo ACTOR_PING = {
    .name = "ping",
    .contextSize = sizeof(PingContext),
    .msgs =
        {
            {&MSG_CREATE, sizeof(PingCreate)},
            {&MSG_PING, 0},
            {NULL},
        },
    .routes =
        {
            {&MSG_CREATE, ping_on_create},
            {&MSG_PING, ping_on_ping},
            {NULL},
        },
};

////////////////////////////////////////////////////////////////////////////////

static ActorMsg *ping_on_create(const ActorMsg *msg) {
  PingCreate *params = msg->params;
  PingContext *ctx = msg->context;
  ctx->max = params->max;
  ctx->count = 0;
  actor_send(msg->me, msg->me, MSG_PING, NULL);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static ActorMsg *ping_on_ping(const ActorMsg *msg) {
  PingContext *ctx = msg->context;

  printf("PING\n");

  if (ctx->count++ < ctx->max) {
    actor_send(msg->me, msg->me, MSG_PING, NULL);
  } else {
    actor_done(msg);
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// MAIN ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main() {
  PingCreate params;
  params.max = 1000;
  return actor_init(&ACTOR_PING, &params);
}
