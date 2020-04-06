#include "msg.h"

#include <stdlib.h>
#include <string.h>

Msg *msg_create(ActorId from, const MsgSpec *spec, void *params) {
  Msg *msg = malloc(sizeof(Msg) + spec->paramsSize);
  msg->from = from;
  msg->spec = spec;
  msg->params = msg + sizeof(Msg);
  memcpy(msg->params, params, spec->paramsSize);
  return msg;
}

void msg_destroy(Msg *msg) { free(msg); }
