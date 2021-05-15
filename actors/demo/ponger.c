////////////////////////////////////////////////////////////////////////////////
// ActorPong
////////////////////////////////////////////////////////////////////////////////

#include "ponger.h"
#include "pinger.h"

ACTOR_IMPL(
    Ponger, 
    { 
      char ignored;
    });

MSG_IMPL(Pong);

ACTOR_ON_START(Ponger, actor, params, state, msg) { debug("Ponger started."); }

ACTOR_ON_RECEIVE(Ponger, actor, params, state, msg) {
  if (msg->type == &Ping) {
    const PingParams *ping = msg->payload;
    PongParams pong = {.num = ping->num};
    actors_send(actor, msg->from, &Pong, &pong);
  }
}

ACTOR_ON_STOP(Ponger, actor, params, state, msg) { debug("Ponger stopped."); }
