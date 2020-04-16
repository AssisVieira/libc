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

#include "queue_ss.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct QueueSS {
  int volatile reader;
  int volatile writer;
  int max;
  void *items[];
} QueueSS;

QueueSS *queue_ss_create(int max) {
  QueueSS *queue = NULL;

  if (max <= 0) return NULL;

  queue = malloc(sizeof(QueueSS) + (sizeof(void *) * max));

  if (queue == NULL) {
    return NULL;
  }

  queue->reader = 0;
  queue->writer = 0;
  queue->max = max + 1;

  return queue;
}

void queue_ss_destroy(QueueSS *queue) { free(queue); }

bool queue_ss_add(QueueSS *queue, void *item) {
  const int newWriter = (queue->writer + 1) % queue->max;
  if (newWriter != queue->reader) {
    queue->items[queue->writer] = item;
    queue->writer = newWriter;
    return true;
  }
  return false;
}

void *queue_ss_get(QueueSS *queue) {
  if (queue->reader != queue->writer) {
    void *item = queue->items[queue->reader];
    queue->reader = (queue->reader + 1) % queue->max;
    return item;
  }
  return NULL;
}
