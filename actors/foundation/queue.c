////////////////////////////////////////////////////////////////////////////////
// Queue
////////////////////////////////////////////////////////////////////////////////

#include "queue.h"

Queue *queue_create(int max) {
  Queue *queue = NULL;

  if (max <= 0) return NULL;

  queue = malloc(sizeof(Queue));
  queue->items = malloc(sizeof(void *) * (max + 1));

  if (queue == NULL) {
    return NULL;
  }

  queue->reader = 0;
  queue->writer = 0;
  queue->max = max + 1;

  return queue;
}

void queue_free(Queue *queue) {
  free(queue->items);
  free(queue);
}

bool queue_add(Queue *queue, void *item) {
  assert(queue != NULL);
  assert(item != NULL);
  bool r = false;
  const int newWriter = (queue->writer + 1) % queue->max;
  if (newWriter != queue->reader) {
    queue->items[queue->writer] = item;
    queue->writer = newWriter;
    r = true;
  }
  return r;
}

bool queue_is_empty(Queue *queue) {
  bool empty = (queue->reader == queue->writer) ? true : false;
  return empty;
}

void * queue_get(Queue *queue) {
  void *item = NULL;
  if (queue->reader != queue->writer) {
    item = queue->items[queue->reader];
    queue->reader = (queue->reader + 1) % queue->max;
  }
  return item;
}
