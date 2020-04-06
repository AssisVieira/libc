#ifndef MSG_SPEC_H
#define MSG_SPEC_H

#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

typedef size_t MsgType;

typedef struct MsgSpec {
  MsgType id;
  char *name;
  size_t paramsSize;
  bool registred;
  mtx_t lock;
} MsgSpec;

void msg_spec_register(const MsgSpec *spec);

/**
 * Defines a new message type that can be addressed by the actors. In
 * addition, it also defines a new type with the content structure of the
 * message. The name of the struct is the result of the concatenation:
 * <MessageName> + Params. If the message name is "CreateUser", the struct
 * name is "CreateUserParams".
 *
 * @param name message name
 * @param paramsName
 */
#define MESSAGE(msgName, params)                         \
  typedef struct msgName##Params params msgName##Params; \
  const MsgSpec msgName = {                              \
      .id = 0,                                           \
      .name = #msgName,                                  \
      .paramsSize = sizeof(msgName##Params),             \
      .registred = false,                                \
  }

#endif