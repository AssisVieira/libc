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
#include <stdint.h>
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

  if (max == 0 || max == SIZE_MAX) return NULL;

  // Adds the dummy node.
  max = max + 1;

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
  queue->max = max;

  return queue;
}

void queue_destroy(Queue *queue) { free(queue); }

bool queue_add(Queue *queue, void *item, bool wait) {
  if (item == NULL) return false;
  mtx_lock(&queue->mutex);
  while (true) {
    size_t newWriter = (queue->writer + 1) % queue->max;
    if (newWriter == queue->reader) {
      if (wait) {
        cnd_wait(&queue->notFull, &queue->mutex);
        continue;
      } else {
        mtx_unlock(&queue->mutex);
        return false;
      }
    }
    queue->items[queue->writer] = item;
    queue->writer = newWriter;
    cnd_signal(&queue->notEmpty);
    break;
  }
  mtx_unlock(&queue->mutex);
  return true;
}

void *queue_get(Queue *queue, bool wait) {
  mtx_lock(&queue->mutex);
  while (queue->reader == queue->writer) {
    if (wait) {
      cnd_wait(&queue->notEmpty, &queue->mutex);
    } else {
      mtx_unlock(&queue->mutex);
      return NULL;
    }
  }
  void *item = queue->items[queue->reader];
  queue->reader = (queue->reader + 1) % queue->max;
  cnd_signal(&queue->notFull);
  mtx_unlock(&queue->mutex);
  return item;
}

size_t queue_count(Queue *queue) {
  size_t count = 0;

  if (queue == NULL) return 0;

  mtx_lock(&queue->mutex);

  if (queue->writer >= queue->reader) {
    // [0][1][2][3][4]
    //     *  *  *
    //  r        w      count = 3 - 0 = 3
    count = queue->writer - queue->reader;
  } else {
    // [0][1][2][3][4]
    //  *        *  *
    //  w     r         count = 5 - 2 + 0 = 3
    count = queue->max - queue->reader + queue->writer;
  }

  mtx_unlock(&queue->mutex);

  return count;
}