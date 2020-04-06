#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "actors.h"
#include "db/db.h"
#include "log/log.h"

////////////////////////////////////////////////////////////////////////////////
/// ACTOR PING INTERFACE////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct ActorPing {
  int actorPong;
  int maxPings;
  int count;
} ActorPing;

const int ACTOR_PING = 1;

static ActorMsg *actorPing_receive(const ActorMsg *msg);
static int actorPing_create(Actor *parent, const char *name, int maxPing);

////////////////////////////////////////////////////////////////////////////////
/// ACTOR PONG INTERFACE////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

enum {
  ACTOR_PONG,
};

static ActorMsg *actorPong_receive(const ActorMsg *msg);
static void actorPong_pong(Actor *from, int to);
static int actorPong_create(Actor *parent, const char *name);

////////////////////////////////////////////////////////////////////////////////
/// ACTOR PING IMPLEMENTATION //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static ActorMsg *actorPing_receive(const ActorMsg *msg) {
  Actor *me = actor_msg_to(msg);
  ActorPing *ctx = actor_context(me);

  switch (actor_msg_type(msg)) {
    case ACTOR_CREATE: {
      printf("[%s] Hello!\n", actor_name(me));

      ActorPing newContext;
      newContext.maxPings = *(int *)actor_msg_params(msg);
      newContext.count = 0;
      newContext.actorPong = actorPong_create(me, "actor-pong");

      return actor_resp(ACTOR_CREATED, &newContext, sizeof(newContext));
    }

    case ACTOR_PING: {
      if (ctx->count < ctx->maxPings) {
        printf("[%s] PING %d\n", actor_name(me), ctx->count++);
        actorPong_pong(me, ctx->actorPong);
      } else {
        kill(getpid(), SIGTERM);
      }
      return NULL;
    }

    case ACTOR_DESTROY: {
      printf("[%s] Bye!\n", actor_name(me));
      return NULL;
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static int actorPing_create(Actor *parent, const char *name, int maxPing) {
  return actor_create(parent, name, actorPing_receive, &maxPing,
                      sizeof(maxPing));
}

////////////////////////////////////////////////////////////////////////////////

static void actorPing_ping(Actor *from, int to) {
  actor_send(from, to, ACTOR_PING, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// ACTOR PONG IMPLEMENTATION //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static ActorMsg *actorPong_receive(const ActorMsg *msg) {
  Actor *me = actor_msg_to(msg);

  switch (actor_msg_type(msg)) {
    case ACTOR_CREATE: {
      printf("[%s] Hello!\n", actor_name(me));
      return NULL;
    }

    case ACTOR_PONG: {
      printf("[%s] PONG\n", actor_name(me));
      return actor_resp(ACTOR_PING, NULL, 0);
    }

    case ACTOR_DESTROY: {
      printf("[%s] Bye!\n", actor_name(me));
      return NULL;
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static int actorPong_create(Actor *parent, const char *name) {
  return actor_create(parent, name, actorPong_receive, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////

static void actorPong_pong(Actor *from, int to) {
  actor_send(from, to, ACTOR_PONG, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  Actor *root = NULL;
  int count = 0;
  int actorPing = -1;
  
  root = actor_init();

  count = atoi(argv[1]);

  actorPing = actorPing_create(root, "actor-ping", count);

  actorPing_ping(root, actorPing);

  return actor_wait(root);
}
