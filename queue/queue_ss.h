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

#ifndef QUEUE_SS_H
#define QUEUE_SS_H

#include <stdbool.h>

typedef struct QueueSS QueueSS;

/**
 * Creates a queue that supports only a single producer and a single consumer.
 *
 * @param count maximum number of items in the queue.
 */
QueueSS *queue_ss_create(int count);

/**
 * Adds a item in the end of the queue. This queue support a single producer.
 */
bool queue_ss_add(QueueSS *queue, void *item);

/**
 * Gets the first item of the queue. This queue support a single consumer.
 */
void *queue_ss_get(QueueSS *queue);

/**
 * Destroys the queue freeing the used memory, but not the items.
 */
void queue_ss_destroy(QueueSS *queue);

#endif
