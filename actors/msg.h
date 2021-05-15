#ifndef MSG_INLCUDE_H
#define MSG_INCLUDE_H

#include "foundation/foundation.h"

typedef struct ActorCell ActorCell;

typedef struct MsgType {
  const char *name;
  size_t size;
} MsgType;

typedef struct Msg {
  const MsgType *type;
  void *payload;
  ActorCell *from;
} Msg;

Msg *msg_create(ActorCell *from, const MsgType *type, const void *payload);
void msg_free(Msg *msg);

/**
 * Defines a new message type that can be addressed by the actors. In
 * addition, it also defines a new type with the content structure of the
 * message. The name of the struct is the result of the concatenation:
 * <MessageName> + Params. If the message name is "CreateUser", the struct
 * name is "CreateUserParams".
 *
 * @param msgName name
 * @param params parameters
 */
#define MSG(msgName, params)                             \
  typedef struct msgName##Params params msgName##Params; \
  extern const MsgType msgName

#define MSG_IMPL(msgName)                    \
  const MsgType msgName = {                  \
      .name = #msgName,                      \
      .size = sizeof(msgName##Params),       \
  }

MSG(Start,   { char ignored; });
MSG(Stop,    { char ignored; });
MSG(Stopped, { char ignored; });

#endif
