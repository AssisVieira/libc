////////////////////////////////////////////////////////////////////////////////
// Queue
////////////////////////////////////////////////////////////////////////////////

#ifndef QUEUE_INCLUDE_H
#define QUEUE_INCLUDE_H

#include "../standard/standard.h"

typedef struct Queue {
  atomic_int reader;
  atomic_int writer;
  int max;
  void **items;
} Queue;

Queue *queue_create(int max);
void queue_free(Queue *queue);
bool queue_add(Queue *queue, void *item);
bool queue_is_empty(Queue *queue);
void * queue_get(Queue *queue);

#endif
