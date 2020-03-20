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

typedef struct Queue Queue;

/**
 * Creates a queue that supports only many producers and many consumers.
 *
 * @param count maximum number of items in the queue.
 */
Queue *queue_create(int max);

/**
 * Adds a item in the end of the queue. This queue support many producers.
 */
bool queue_add(Queue *queue, void *item);

/**
 * Gets the first item of the queue. This queue support many consumers.
 */
void *queue_get(Queue *queue);

/**
 * Gets the number of queue items.
 */
int queue_count(Queue *queue);

/**
 * Destroys the queue freeing the used memory, but not the items.
 */
void queue_destroy(Queue *queue);

#endif
