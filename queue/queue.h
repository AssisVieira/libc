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

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Queue Queue;

/**
 * Creates a bounded queue that supports many producers and many consumers.
 * This implementation uses only one mutex to synchronize the add and get
 * operations.
 *
 * @param max maximum number of items in the queue.
 */
Queue *queue_create(size_t max);

/**
 * Adds a item in the end of the queue. This queue support many producers.
 * If the queue is full, the call will be blocked until a item be removed.
 */
void queue_add(Queue *queue, void *item);

/**
 * Gets and removes the first item in the queue. This queue support many
 * consumers. If the queue is empty, the call will be blocked until a item be
 * added.
 */
void *queue_get(Queue *queue);

/**
 * Destroys the queue freeing the used memory, except the added items.
 */
void queue_destroy(Queue *queue);

#endif
