#include "actors.h"

#include <string.h>

#include "queue/queue.h"
#include "vetor/vetor.h"
#include "threads/thpool.h"
#include <threads.h>
#include <stdbool.h>

typedef struct thpool_ ThreadPool;

////////////////////////////////////////////////////////////////////////////////

typedef struct Message {
  int type;
  int size;
  char *content[];
} Message;

/**
 * Create a message, copying the content.
 */
static Message *message_create(int type, const void *content, int size);

/**
 * Free memory used by the message.
 */
static void message_destroy(Message *message);

////////////////////////////////////////////////////////////////////////////////

typedef struct Actor {
  char *name;
  Queue *messages;
  ActorHandler handler;
  bool volatile scheduled;
} Actor;

/**
 * Create a instance of actor.
 *
 * @param name      actor's name, it's util for identify the actor's instance at
 *                  the system.
 * @param max       maximum number the messages in the actor's queue.
 * @param handler   function with the logic of the actor.
 */
static Actor *actor_create(const char *name, int max, ActorHandler handler);

/**
 * Send a message to the actor's queue, copying the content.
 */
static void actor_send(Actor *actor, int type, const void *content, int size);

static void actor_destroy(Actor *actor);

////////////////////////////////////////////////////////////////////////////////

typedef struct Actors {
  ThreadPool *threads;
  Vetor *actors;
  Queue *jobs;
} Actors;

////////////////////////////////////////////////////////////////////////////////

static Message *message_create(int type, const void *content, int size) {
  Message *message = malloc(sizeof(Message) + size);
  message->type = type;
  message->size = size;
  memcpy(message->content, content, size);
  return message;
}

////////////////////////////////////////////////////////////////////////////////

static void message_destroy(Message *message) { free(message); }

////////////////////////////////////////////////////////////////////////////////

static Actor *actor_create(const char *name, int max, ActorHandler handler) {
  Actor *actor = malloc(sizeof(Actor));
  actor->name = strdup(name);
  actor->handler = handler;
  actor->messages = queue_create(max);
  return actor;
}

////////////////////////////////////////////////////////////////////////////////

static void actor_send(Actor *actor, int type, const void *content, int size) {
  Message *message = message_create(type, content, size);
  while (!queue_add(actor->messages, message)) thrd_yield();
}

////////////////////////////////////////////////////////////////////////////////

static void actor_worker(void *arg) {
  Actor *actor = arg;

  while (true) {
    Message *msg = queue_get(actor->messages);

    if (msg == NULL) {
      break;
    }

    actor->handler(msg->type, msg->content, msg->size);

    message_destroy(msg);
  }

  actor->scheduled = false;
}

////////////////////////////////////////////////////////////////////////////////

static void actor_destroy(Actor *actor) {
  free(actor->name);
  queue_destroy(actor->messages);
  free(actor);
}
////////////////////////////////////////////////////////////////////////////////

static void actors_worker(void *arg) {
  Actors *actors = arg;
  while (1) {
    Actor *actor = queue_get(actors->jobs);
    if (actor == NULL) {
      thrd_yield();
      continue;
    }
    thpool_add_work(actors->threads, actor_worker, actor);
  }
}

////////////////////////////////////////////////////////////////////////////////

Actors *actors_create(int threads) {
  Actors *actors = malloc(sizeof(Actors));
  actors->threads = thpool_init(threads);
  actors->actors = vetor_criar(100);
  actors->jobs = queue_create(100);

  thpool_add_work(actors->threads, actors_worker, actors);

  return actors;
}

////////////////////////////////////////////////////////////////////////////////

void actors_wait(Actors *actors) { thpool_wait(actors->threads); }

////////////////////////////////////////////////////////////////////////////////

void actors_send(Actors *actors, int to, int type, const void *msg,
                 size_t size) {
  Actor *toActor = vetor_item(actors->actors, to);
  actor_send(toActor, type, msg, size);
  if (__sync_bool_compare_and_swap(&toActor->scheduled, false, true)) {  // CAS
    queue_add(actors->jobs, toActor);
  }
}

////////////////////////////////////////////////////////////////////////////////

int actors_actorCreate(Actors *actors, const char *name, int max,
                       ActorHandler handler) {
  Actor *actor = actor_create(name, max, handler);
  return vetor_add(actors->actors, actor);
}

////////////////////////////////////////////////////////////////////////////////

int actors_actorId(Actors *actors, const char *name) {
  for (int i = 0; i < vetor_qtd(actors->actors); i++) {
    Actor *actor = vetor_item(actors->actors, i);
    if (strcmp(actor->name, name) == 0) {
      return i;
    }
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

void actors_destroy(Actors *actors) {
  thpool_destroy(actors->threads);
  for (int i = 0; i < vetor_qtd(actors->actors); i++) {
    Actor *actor = vetor_item(actors->actors, i);
    actor_destroy(actor);
  }
  vetor_destruir(actors->actors);
  free(actors);
}
