#ifndef RING_H
#define RING_H

#include <stdbool.h>

typedef struct Ring Ring;
typedef struct Msg Msg;

bool ring_isEmpty(Ring *ring);
bool ring_isFull(Ring *ring);
void ring_free(Ring *ring);
Ring *ring_new(int count);
void ring_add(Ring *ring, const Msg * item);
Msg * ring_get(Ring *ring); 

#endif

