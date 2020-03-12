#include "actors.h"

#include <string.h>

#include "vetor/vetor.h"

typedef void Queue;
typedef Vetor Vector;
typedef void ThreadPool;

////////////////////////////////////////////////////////////////////////////////

typedef struct Message {
  int type;
  int size;
  void *content;
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
  Vector *actors;
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
  queue_add(actor->messages, message);
}

////////////////////////////////////////////////////////////////////////////////

static void actor_destroy(Actor *actor) {
  free(actor->name);
  queue_destroy(actor->messages);
  free(actor);
}

////////////////////////////////////////////////////////////////////////////////

Actors *actors_create(int min, int max, int factor) {
  Actors *actors = malloc(sizeof(Actors));
  actors->threads = threadspoll_create(min, max, factor);
  actors->actors = vector_create(100);
  actors->jobs = queue_create(100);
  return actors;
}

////////////////////////////////////////////////////////////////////////////////

void actors_send(Actors *actors, int to, int type, const void *msg,
                 size_t size);

////////////////////////////////////////////////////////////////////////////////

void actors_execute(Actors *actors);

////////////////////////////////////////////////////////////////////////////////

int actors_actorCreate(Actors *actors, const char *name, int max,
                       ActorHandler handler);

////////////////////////////////////////////////////////////////////////////////

int actors_actorId(Actors *actors, const char *name);

////////////////////////////////////////////////////////////////////////////////

void actors_destroy(Actors *actors);