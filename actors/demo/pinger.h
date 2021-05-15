#ifndef PINGER_INCLUDE_H
#define PINGER_INCLUDE_H

#include "actors/actors.h"

ACTOR(Pinger, {
  int maxPings;
  int numPongers;
});

MSG(Ping, { int num; });

#endif
