#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <limits.h>
#include <threads.h>
#include <assert.h>
#include <string.h>

typedef struct Queue {
  int volatile reader;
  int volatile writer;
  mtx_t mtxReader;
  mtx_t mtxWriter;
  int max;
  void *items[];
} Queue;

Queue *queue_create(int max) {
  Queue *queue = malloc(sizeof(Queue) + (sizeof(void *) * max));

  if (queue == NULL) {
    return NULL;
  }

  queue->reader = 0;
  queue->writer = 0;
  queue->max = max;
  mtx_init(&queue->mtxReader, mtx_plain);
  mtx_init(&queue->mtxWriter, mtx_plain);

  return queue;
}

void queue_destroy(Queue *queue) { free(queue); }

bool queue_add(Queue *queue, void *item) {
  bool r = false;

  mtx_lock(&queue->mtxWriter);
  int newWriter = (queue->writer + 1) % queue->max;
  if (newWriter != queue->reader) {
    queue->items[queue->writer] = item;
    queue->writer = newWriter;
    r = true;
  }
  mtx_unlock(&queue->mtxWriter);

  return r;
}

void *queue_get(Queue *queue) {
  void *item = NULL;
  mtx_lock(&queue->mtxReader);
  if (queue->reader != queue->writer) {
    item = queue->items[queue->reader];
    queue->reader = (queue->reader + 1) % queue->max;
  }
  mtx_unlock(&queue->mtxReader);
  return item;
}
