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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "queue_ss.h"

static void testCreate() {
  {
    QueueSS *queue = queue_ss_create(-1);
    assert(queue == NULL);
  }
  {
    QueueSS *queue = queue_ss_create(0);
    assert(queue == NULL);
  }
  {
    QueueSS *queue = queue_ss_create(1);
    assert(queue != NULL);
    queue_ss_destroy(queue);
  }
}

static void testAdd() {
  QueueSS *queue = queue_ss_create(3);

  assert(queue_ss_add(queue, "a") == true);
  assert(queue_ss_add(queue, "b") == true);
  assert(queue_ss_add(queue, "c") == true);
  assert(queue_ss_add(queue, "d") == false);

  queue_ss_destroy(queue);
}

static void testGet() {
  QueueSS *queue = queue_ss_create(3);

  assert(queue_ss_get(queue) == NULL);

  assert(queue_ss_add(queue, "a") == true);
  assert(queue_ss_add(queue, "b") == true);

  assert(strcmp(queue_ss_get(queue), "a") == 0);

  assert(strcmp(queue_ss_get(queue), "b") == 0);

  assert(queue_ss_get(queue) == NULL);

  queue_ss_destroy(queue);
}

static int testConsumer(void *arg) {
  QueueSS *queue = arg;
  char *item = NULL;
  int count = 0;

  while (true) {
    ++count;

    while ((item = queue_ss_get(queue)) == NULL) thrd_yield();

    if (strcmp(item, "abc")) break;

    free(item);
  }

  assert(strcmp(item, "end") == 0);
  free(item);

  assert(queue_ss_get(queue) == NULL);

  assert(count == 1000001);

  return 0;
}

static int testProducer(void *arg) {
  QueueSS *queue = arg;

  for (int i = 0; i < 1000000; i++) {
    char *item = strdup("abc");
    while (!queue_ss_add(queue, item)) thrd_yield();
  }

  char *item = strdup("end");
  while (!queue_ss_add(queue, item)) thrd_yield();

  return 0;
}

static void testSingleProducerSingleConsumer() {
  thrd_t tProducer;
  thrd_t tConsumer;
  QueueSS *queue = queue_ss_create(100);

  thrd_create(&tProducer, testProducer, queue);
  thrd_create(&tConsumer, testConsumer, queue);

  thrd_join(tProducer, NULL);
  thrd_join(tConsumer, NULL);

  queue_ss_destroy(queue);
}

int main() {
  testCreate();
  testAdd();
  testGet();
  testSingleProducerSingleConsumer();
  return 0;
}