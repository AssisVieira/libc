////////////////////////////////////////////////////////////////////////////////
// ActorPing
////////////////////////////////////////////////////////////////////////////////

#include "pinger.h"

#include "ponger.h"

ACTOR_IMPL(
    Pinger,
    {
      int numPings;
      int currPonger;
      ActorCell **pongers;
    });

MSG_IMPL(Ping);

ACTOR_ON_START(Pinger, actor, params, state, msg) {
  state->numPings = 0;
  state->currPonger = 0;

  state->pongers = malloc(sizeof(ActorCell *) * params->numPongers);

  for (int i = 0; i < params->numPongers; i++) {
    char name[32] = {0};
    snprintf(name, sizeof(name), "Ponger %d", i);
    state->pongers[i] = actors_child_new(actor, name, &Ponger, NULL);
    actors_send(actor, state->pongers[i], &Ping,
                &(PingParams){.num = state->numPings});
  }

  debug("Pinger started.");
}

ACTOR_ON_RECEIVE(Pinger, actor, params, state, msg) {
  if (msg->type == &Pong) {
    if (state->numPings >= params->maxPings) {
      actors_stop_self(actor);
      return;
    }

    state->numPings++;

    do {
      state->currPonger = (state->currPonger + 1) % params->numPongers;
    } while (state->pongers[state->currPonger] == NULL);

    actors_send(actor, state->pongers[state->currPonger], &Ping,
                &(PingParams){.num = state->numPings});
  } else if (msg->type == &Stopped) {
    for (int i = 0; i < params->numPongers; i++) {
      if (state->pongers[i] == msg->from) {
        state->pongers[i] = NULL;
      }
    }
  }
}

ACTOR_ON_STOP(Pinger, actor, params, state, msg) {
  free(state->pongers);
  debug("Pinger stopped.");
}
