/*******************************************************************************
 *   Copyright 2020 Assis Vieira
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************/

#include "queue.h"

#include <assert.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

typedef struct Queue {
  size_t reader;
  size_t writer;
  size_t max;
  mtx_t mutex;
  cnd_t notEmpty;
  cnd_t notFull;
  void *items[];
} Queue;

Queue *queue_create(size_t max) {
  Queue *queue = NULL;

  if (max <= 0) return NULL;

  queue = malloc(sizeof(Queue) + (sizeof(void *) * max));

  if (queue == NULL) {
    return NULL;
  }

  if (mtx_init(&queue->mutex, mtx_plain)) {
    free(queue);
    perror("mtx_init()\n");
    return NULL;
  }

  if (cnd_init(&queue->notEmpty)) {
    free(queue);
    perror("cnd_init()\n");
    return NULL;
  }

  if (cnd_init(&queue->notFull)) {
    free(queue);
    perror("cnd_init()\n");
    return NULL;
  }

  queue->reader = 0;
  queue->writer = 0;
  queue->max = max + 1;

  return queue;
}

void queue_destroy(Queue *queue) { free(queue); }

void queue_add(Queue *queue, void *item) {
  mtx_lock(&queue->mutex);
  while (true) {
    size_t newWriter = (queue->writer + 1) % queue->max;
    if (newWriter == queue->reader) {
      cnd_wait(&queue->notFull, &queue->mutex);
      continue;
    }
    queue->items[queue->writer] = item;
    queue->writer = newWriter;
    cnd_signal(&queue->notEmpty);
    break;
  }
  mtx_unlock(&queue->mutex);
}

void *queue_get(Queue *queue) {
  mtx_lock(&queue->mutex);
  while (queue->reader == queue->writer) {
    cnd_wait(&queue->notEmpty, &queue->mutex);
  }
  void *item = queue->items[queue->reader];
  queue->reader = (queue->reader + 1) % queue->max;
  cnd_signal(&queue->notFull);
  mtx_unlock(&queue->mutex);
  return item;
}
