#ifndef MSG_H
#define MSG_H

#include "actor.h"
#include "msg_type.h"

typedef struct Msg {
  const MsgType *type;
  Actor *from;
  void *params;
} Msg;

Msg *msg_create(Actor *from, const MsgType *type, const void *params,
                size_t paramsSize);

void msg_destroy(Msg *msg);

#endif