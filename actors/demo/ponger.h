#ifndef PONGER_INCLUDE_H
#define PONGER_INCLUDE_H

#include "actors/actors.h"

ACTOR(Ponger, {
  char ignored;
});

MSG(Pong, { int num; });

#endif
