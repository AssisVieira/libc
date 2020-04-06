#ifndef MSG_H
#define MSG_H

#include "actor.h"
#include "msg_spec.h"

typedef struct Msg {
  const MsgSpec *spec;
  ActorId from;
  void *params;
} Msg;

Msg *msg_create(ActorId from, const MsgSpec *spec, void *params);

void msg_destroy(Msg *msg);

#endif