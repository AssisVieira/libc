#include <stdatomic.h>

#include "msg_type.h"

static atomic_size_t MSG_TYPE_COUNT = 0;

void msg_type_register(const MsgType *type) {
  MsgType *typePtr = (MsgType *)type;

  // Checking to avoid the lock...
  if (typePtr->registred) return;

  mtx_lock(&typePtr->lock);
  if (!typePtr->registred) {
    typePtr->id = MSG_TYPE_COUNT++;
    typePtr->registred = true;
  }
  mtx_unlock(&typePtr->lock);
}
