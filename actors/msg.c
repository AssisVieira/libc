#include "msg.h"

#include <stdlib.h>
#include <string.h>

Msg *msg_create(Actor *from, const MsgType *type, const void *params,
                size_t paramsSize) {
  Msg *msg = malloc(sizeof(Msg) + paramsSize);
  msg->from = from;
  msg->type = type;
  msg->params = msg + sizeof(Msg);
  memcpy(msg->params, params, paramsSize);
  return msg;
}

void msg_destroy(Msg *msg) { free(msg); }
