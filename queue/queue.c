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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

typedef struct Node {
  void *value;
  struct Node *next;
} Node;

typedef struct Queue {
  volatile int count;
  int max;
  mtx_t mtxTail;
  mtx_t mtxHead;
  Node *head;
  Node *tail;
} Queue;

Queue *queue_create(int max) {
  if (max <= 0) return NULL;

  Queue *queue = malloc(sizeof(Queue));

  if (queue == NULL) return NULL;

  queue->count = 0;
  queue->max = max;

  if (mtx_init(&queue->mtxHead, mtx_plain)) {
    free(queue);
    return NULL;
  }

  if (mtx_init(&queue->mtxTail, mtx_plain)) {
    free(queue);
    return NULL;
  }

  Node *emptyNode = malloc(sizeof(Node));

  if (emptyNode == NULL) {
    free(queue);
    return NULL;
  }

  emptyNode->value = NULL;
  emptyNode->next = NULL;

  queue->head = emptyNode;
  queue->tail = emptyNode;

  return queue;
}

int queue_count(Queue *queue) { return queue->count; }

void queue_destroy(Queue *queue) {
  while (queue->head != NULL) {
    Node *node = queue->head;
    queue->head = node->next;
    free(node);
  }
  free(queue);
}

bool queue_add(Queue *queue, void *item) {
  do {
    int oldCount = queue->count;

    if (oldCount >= queue->max) return false;

    if (__sync_bool_compare_and_swap(&queue->count, oldCount, oldCount + 1)) {
      break;
    }
  } while (true);

  Node *node = malloc(sizeof(Node));

  if (node == NULL) return false;

  node->value = item;
  node->next = NULL;

  mtx_lock(&queue->mtxTail);
  queue->tail->next = node;
  queue->tail = node;
  mtx_unlock(&queue->mtxTail);

  return true;
}

void *queue_get(Queue *queue) {
  mtx_lock(&queue->mtxHead);
  Node *node = queue->head;
  Node *newHead = node->next;
  if (newHead == NULL) {
    mtx_unlock(&queue->mtxHead);
    return NULL;
  }
  Node *value = newHead->value;
  queue->head = newHead;
  mtx_unlock(&queue->mtxHead);
  __sync_fetch_and_sub(&queue->count, 1);
  free(node);
  return value;
}
