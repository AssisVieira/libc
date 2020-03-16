#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct Queue Queue;

Queue *queue_create(int count);
bool queue_add(Queue *queue, void *item);
void * queue_get(Queue *queue); 
void queue_destroy(Queue *queue);

#endif

