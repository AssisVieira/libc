#ifndef ACTORS_H
#define ACTORS_H

#include <stddef.h>

typedef struct Actors Actors;
typedef void (*ActorHandler)(int type, const void *msg, int size);

/**
 * Create a instance of actor system.
 *
 * @param threads number of threads.
 */
Actors *actors_create(int threads);

/**
 * Send a async message to a actor.
 *
 * @param to    actor's id.
 * @param type  message type.
 * @param msg   message content.
 * @param size  size of message content, in bytes.
 */
void actors_send(Actors *actors, int to, int type, const void *msg,
                 size_t size);

/**
 * Create a instance of a actor.
 *
 * @param name     actor's name
 * @param max      maximum number the messages in the actor's queue.
 * @param handler  function with the logic of the actor.
 */
int actors_actorCreate(Actors *actors, const char *name, int max,
                       ActorHandler handler);

/**
 * Obtain the actor id.
 *
 * @param name  actor's name.
 */
int actors_actorId(Actors *actors, const char *name);

/**
 * Destroy the actor system.
 */
void actors_destroy(Actors *actors);

void actors_wait(Actors *actors);

#endif