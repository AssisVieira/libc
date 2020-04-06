#include "msg_spec.h"

#include <stdatomic.h>

static atomic_size_t MSG_TYPE_COUNT = 0;

void msg_spec_register(const MsgSpec *spec) {
  MsgSpec *specPtr = (MsgSpec *)spec;

  // Checking to avoid the lock...
  if (specPtr->registred) return;

  mtx_lock(&specPtr->lock);
  if (!specPtr->registred) {
    specPtr->id = MSG_TYPE_COUNT++;
    specPtr->registred = true;
  }
  mtx_unlock(&specPtr->lock);
}
