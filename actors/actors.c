#include "actors.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "queue/queue.h"
#include "vetor/vetor.h"

////////////////////////////////////////////////////////////////////////////////
/// ACTOR MSG DEFINITIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct ActorMsg {
  Actor *from;
  Actor *to;
  int type;
  int size;
  char *params[];
} ActorMsg;

////////////////////////////////////////////////////////////////////////////////
/// ACTOR DEFINITIONS //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct Actor {
  int id;
  char *name;
  Queue *msgs;
  thrd_t thread;
  ActorReceive receive;
  void *context;
  struct Actor *parent;
  Vetor *children;
} Actor;

/**
 * Executes the actor until he is destroyed.
 *
 * @param arg  actor.
 */
static int actor_worker(void *arg);

////////////////////////////////////////////////////////////////////////////////
/// ACTOR IMPLEMENTATION ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void actor_close(Actor *actor) {
  for (int i = 0; i < vetor_qtd(actor->children); i++) {
    if (vetor_item(actor->children, i) != NULL) {
      actor_destroy(actor, i);
    }
  }

  printf("[%s] Bye!\n", actor->name);

  vetor_destruir(actor->children);
  free(actor->name);
  free(actor->context);
  free(actor);
}

////////////////////////////////////////////////////////////////////////////////

Actor *actor_init() {
  Actor *actor = NULL;
  sigset_t mask;

  sigfillset(&mask);

  if (pthread_sigmask(SIG_SETMASK, &mask, NULL)) {
    fprintf(stderr, "pthread_sigmask()\n");
    return NULL;
  }

  actor = malloc(sizeof(Actor));
  actor->id = -1;
  actor->name = strdup("actor-main");
  actor->receive = NULL;
  actor->msgs = NULL;
  actor->context = NULL;
  actor->children = vetor_criar(32);
  actor->parent = NULL;
  actor->thread = thrd_current();

  return actor;
}

////////////////////////////////////////////////////////////////////////////////

int actor_wait(Actor *actor) {
  sigset_t mask;
  int sig;
  sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGINT);

  if (sigwait(&mask, &sig)) {
    fprintf(stderr, "sigwait()\n");
    return -1;
  }

  actor_close(actor);

  // Awaits the last thread, the thread that throwed the SIGTERM.
  // This just prevents valgrind from alerting us about memory leaks.
  sleep(1);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int actor_create(Actor *parent, const char *name, ActorReceive receive,
                 const void *params, int size) {
  Actor *actor = NULL;

  actor = malloc(sizeof(Actor));
  actor->id = 0;
  actor->name = strdup(name);
  actor->receive = receive;
  actor->msgs = queue_create(ACTOR_QUEUE_SIZE);
  actor->context = NULL;
  actor->children = vetor_criar(32);
  actor->parent = parent;

  if (thrd_create(&actor->thread, actor_worker, actor)) {
    return -1;
  }

  actor->id = vetor_add(parent->children, actor);

  actor_send(parent, actor->id, ACTOR_CREATE, params, size);

  return actor->id;
}

////////////////////////////////////////////////////////////////////////////////

void actor_send(Actor *from, int to, int type, const void *params,
                size_t size) {
  Actor *actorTo = vetor_item(from->children, to);
  ActorMsg *msg = actor_msg_create(from, actorTo, type, params, size);
  while (!queue_add(actorTo->msgs, msg)) thrd_yield();
}

////////////////////////////////////////////////////////////////////////////////

static int actor_worker(void *arg) {
  Actor *me = arg;
  Actor *parent = me->parent;
  ActorMsg *msg = NULL;

  sigset_t mask;
  sigfillset(&mask);
  if (sigprocmask(SIG_SETMASK, &mask, NULL)) {
    fprintf(stderr, "sigprocmask()\n");
    return -1;
  }

  while (true) {
    while ((msg = queue_get(me->msgs)) == NULL) {
      thrd_yield();
      int sig;
      printf("[%s] wait\n", me->name);
      sigwait(&mask, &sig);
      printf("[%s] wake up\n", me->name);
    }

    if (msg->type == ACTOR_DESTROY) {
      for (int i = 0; i < vetor_qtd(me->children); i++) {
        Actor *child = vetor_item(me->children, i);
        if (child == NULL) continue;
        actor_destroy(me, i);
      }
    }

    if (msg->type == ACTOR_KILLME) {
      actor_destroy(me, msg->from->id);
    }

    ActorMsg *resp = me->receive(msg);

    if (resp != NULL) {
      if (msg->type == ACTOR_CREATE) {
        me->context = malloc(resp->size);
        memcpy(me->context, resp->params, resp->size);
      }

      if (msg->from->msgs != NULL) {
        resp->from = me;
        resp->to = msg->from;
        queue_add(msg->from->msgs, resp);
      } else {
        actor_msg_destroy(resp);
      }
    }

    if (msg->type == ACTOR_DESTROY) {
      break;
    }

    actor_msg_destroy(msg);
  }

  do {
    // Destroys the last message: ACTOR_DESTROY ...
    actor_msg_destroy(msg);
  } while ((msg = queue_get(me->msgs)) != NULL);

  if (parent != NULL) {
    vetor_inserir(parent->children, me->id, NULL);
  }

  queue_destroy(me->msgs);
  vetor_destruir(me->children);
  free(me->name);
  free(me->context);
  free(me);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

ActorMsg *actor_resp(int type, const void *params, size_t size) {
  return actor_msg_create(NULL, NULL, type, params, size);
}

////////////////////////////////////////////////////////////////////////////////

void *actor_context(Actor *actor) { return actor->context; }

////////////////////////////////////////////////////////////////////////////////

const char *actor_name(Actor *actor) { return actor->name; }

////////////////////////////////////////////////////////////////////////////////

void actor_exit(Actor *actor) {
  actor_send(actor->parent, actor->id, ACTOR_DESTROY, NULL, 0);

  // Checks whether the actor is the only primary actor.
  // If it is, then finish the process.
  if (actor->parent->id == -1) {
    int primaryActors = 0;
    for (int i = 0; i < vetor_qtd(actor->parent->children); i++) {
      Actor *child = vetor_item(actor->parent->children, i);
      if (child == NULL) continue;
      primaryActors++;
    }
    if (primaryActors == 0) {
      pthread_kill(actor->parent->thread, SIGTERM);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void actor_destroy(Actor *parent, int actorId) {
  Actor *actor = vetor_item(parent->children, actorId);

  actor_send(parent, actorId, ACTOR_DESTROY, NULL, 0);

  thrd_join(actor->thread, NULL);
}

////////////////////////////////////////////////////////////////////////////////
/// ACTOR MSG IMPLEMENTATION ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ActorMsg *actor_msg_create(Actor *from, Actor *to, int type, const void *params,
                           int size) {
  ActorMsg *msg = malloc(sizeof(ActorMsg) + size);
  msg->from = from;
  msg->to = to;
  msg->type = type;
  msg->size = size;
  memcpy(msg->params, params, size);
  return msg;
}

////////////////////////////////////////////////////////////////////////////////

Actor *actor_msg_from(const ActorMsg *msg) { return msg->from; }

////////////////////////////////////////////////////////////////////////////////

Actor *actor_msg_to(const ActorMsg *msg) { return msg->to; }

////////////////////////////////////////////////////////////////////////////////

int actor_msg_type(const ActorMsg *msg) { return msg->type; }

////////////////////////////////////////////////////////////////////////////////

const void *actor_msg_params(const ActorMsg *msg) { return msg->params; }

////////////////////////////////////////////////////////////////////////////////

void actor_msg_destroy(ActorMsg *message) { free(message); }