////////////////////////////////////////////////////////////////////////////////
// Msg
////////////////////////////////////////////////////////////////////////////////

#include "msg.h"

MSG_IMPL(Start);
MSG_IMPL(Stop);
MSG_IMPL(Stopped);

Msg *msg_create(ActorCell *from, const MsgType *type, const void *payload) {
  Msg *msg = malloc(sizeof(Msg));
  msg->payload = calloc(1, type->size);
  msg->type = type;
  msg->from = from;

  if (payload != NULL) {
    memcpy(msg->payload, payload, type->size);
  }

  return msg;
}

void msg_free(Msg *msg) {
  free(msg->payload);
  free(msg);
}
