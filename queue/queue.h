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
 * Creates a bounded queue that supports  many producers and many consumers.
 * This implementation uses only one mutex to synchronize the add and get
 * operations.
 *
 * @param max maximum number of items in the queue.
 */
Queue *queue_create(size_t max);

/**
 * Adds a item in the end of the queue. This queue support one or more
 * producers.
 *
 * @param queue queue of items.
 * @param item item to be added in the queue. The item cannot be NULL.
 * @param wait true, if the call should be blocked until the item is added. In
 * this case, the function always returns true. false, if the call should not be
 * blocked. In this second case, the function can return true or false.
 * @return true, if the item was added, false, if the queue is full.
 */
bool queue_add(Queue *queue, void *item, bool wait);

/**
 * Obtains and removes the first item in the queue. This queue support one or
 * more consumers.
 *
 * @param queue queue of items.
 * @param wait true, if the call should be blocked until an item is available.
 * In this case, the function always returns an item. false, if the call should
 * not be blocked. In this second case, the function return NULL, if the
 * queue is empty or an item, otherwise.
 * @return item or NULL, if the queue is empty.
 */
void *queue_get(Queue *queue, bool wait);

/**
 * Destroys the queue freeing the used memory, except the added items.
 */
void queue_destroy(Queue *queue);

/**
 * Obtain number of items in the queue.
 */
size_t queue_count(Queue *queue);

#endif
