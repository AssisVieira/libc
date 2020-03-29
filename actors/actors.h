#ifndef ACTORS_H
#define ACTORS_H

#include <stdbool.h>
#include <stddef.h>

#define ACTOR_QUEUE_SIZE 1000

////////////////////////////////////////////////////////////////////////////////
/// ACTOR //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct Actor Actor;
typedef struct ActorMsg ActorMsg;

typedef ActorMsg *(*ActorReceive)(const ActorMsg *msg);

enum {
  ACTOR_CREATE = -1,
  ACTOR_CREATED = -2,
  ACTOR_DESTROY = -3,
  ACTOR_KILLME = -4,
};

/**
 * Creates the root actor. This actor is responsible for creating other actors.
 * This actor only sends messages, he cannot handle the responses. This must be
 * the first function to be called.
 */
Actor *actor_init();

/**
 * Waits all actors to complete your work.
 */
int actor_wait(Actor *actor);

/**
 * Destroys the actor. After calling this function, messages sent to the actor
 * will be rejected. Messages sent before this function will still be delivered
 * to the actor.
 */
void actor_killme(Actor *actor);

/**
 * Sends a message to a actor. If the actor's queue is full, this function will
 * block until a message be consumed by actor and the new message can be sent.
 *
 * @param to      actor's id.
 * @param type    message type.
 * @param params  message params.
 * @param size    size of message content, in bytes.
 */
void actor_send(Actor *from, int to, int type, const void *params, size_t size);

/**
 * Create a response message.
 */
ActorMsg *actor_resp(int type, const void *params, size_t size);

/**
 * Creates a actor.
 *
 * @param parent    actor who is calling this function.
 * @param name      actor's name.
 * @param receiver  function with the logic of the actor.
 * @param params    message params.
 * @param size      size of message params, in bytes.
 * @return          id of the created actor.
 */
int actor_create(Actor *parent, const char *name, ActorReceive receive,
                 const void *params, int size);

/**
 * Sends the message ACTOR_DESTROY to the actor, waits for all previously sent
 * messages to be handled, destroys the actor and frees the used memory. After
 * calling this function, all messages send to the actor will be rejected.
 */
void actor_destroy(Actor *parent, int actor);

/**
 * Obtain the actor's context.
 */
void *actor_context(Actor *actor);

/**
 * Obtain the actor's name.
 */
const char *actor_name(Actor *actor);

////////////////////////////////////////////////////////////////////////////////
/// ACTOR MSG //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Create a message.
 */
ActorMsg *actor_msg_create(Actor *from, Actor *to, int type, const void *params,
                           int size);

Actor *actor_msg_to(const ActorMsg *msg);

Actor *actor_msg_from(const ActorMsg *msg);

int actor_msg_type(const ActorMsg *msg);

int actor_msg_size(const ActorMsg *msg);

const void *actor_msg_params(const ActorMsg *msg);

/**
 * Free memory used by the message.
 */
void actor_msg_destroy(ActorMsg *msg);

#endif