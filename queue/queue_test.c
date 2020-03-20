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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

static void testCreate() {
  {
    Queue *queue = queue_create(-1);
    assert(queue == NULL);
  }
  {
    Queue *queue = queue_create(0);
    assert(queue == NULL);
  }
  {
    Queue *queue = queue_create(1);
    assert(queue != NULL);
    assert(queue_count(queue) == 0);
    queue_destroy(queue);
  }
}

static void testAdd() {
  Queue *queue = queue_create(3);

  assert(queue_add(queue, "a") == true);
  assert(queue_count(queue) == 1);

  assert(queue_add(queue, "b") == true);
  assert(queue_count(queue) == 2);

  assert(queue_add(queue, "c") == true);
  assert(queue_count(queue) == 3);

  assert(queue_add(queue, "d") == false);
  assert(queue_count(queue) == 3);

  queue_destroy(queue);
}

static void testGet() {
  Queue *queue = queue_create(3);

  assert(queue_get(queue) == NULL);
  assert(queue_count(queue) == 0);

  assert(queue_add(queue, "a") == true);
  assert(queue_add(queue, "b") == true);
  assert(queue_count(queue) == 2);

  assert(strcmp(queue_get(queue), "a") == 0);
  assert(queue_count(queue) == 1);

  assert(strcmp(queue_get(queue), "b") == 0);
  assert(queue_count(queue) == 0);

  assert(queue_get(queue) == NULL);
  assert(queue_count(queue) == 0);

  queue_destroy(queue);
}

static int testFastConsumer(void *arg) {
  Queue *queue = arg;
  char *item = NULL;

  while (true) {
    while ((item = queue_get(queue)) == NULL) thrd_yield();

    if (strcmp(item, "abc")) break;

    free(item);
  }

  assert(strcmp(item, "end") == 0);
  free(item);

  return 0;
}

static int testSlowConsumer(void *arg) {
  Queue *queue = arg;
  char *item = NULL;

  while (true) {
    // sleep 1ms
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000000}, NULL);

    while ((item = queue_get(queue)) == NULL) thrd_yield();

    if (strcmp(item, "abc")) break;

    free(item);
  }

  assert(strcmp(item, "end") == 0);
  free(item);

  return 0;
}

static int testSlowProducer(void *arg) {
  Queue *queue = arg;

  for (int i = 0; i < 10000; i++) {
    // sleep 0.5ms
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 500000}, NULL);

    char *item = strdup("abc");
    while (!queue_add(queue, item)) thrd_yield();

    fprintf(stdout, "(%d) i = %d\n", queue_count(queue), i);
    fflush(stdout);
  }

  return 0;
}

static int testFastProducer(void *arg) {
  Queue *queue = arg;

  for (int i = 0; i < 10000; i++) {
    char *item = strdup("abc");
    while (!queue_add(queue, item)) thrd_yield();

    fprintf(stdout, "(%d) i = %d\n", queue_count(queue), i);
    fflush(stdout);
  }

  return 0;
}

static void testSingleProducerSingleConsumer(int (*fnProducer)(void*), int (*fnConsumer)(void*)) {
  thrd_t tProducer;
  thrd_t tConsumer;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer, fnProducer, queue);
  thrd_create(&tConsumer, fnConsumer, queue);

  thrd_join(tProducer, NULL);

  char *item = strdup("end");
  while (!queue_add(queue, item)) thrd_yield();

  thrd_join(tConsumer, NULL);

  assert(queue_count(queue) == 0);

  queue_destroy(queue);
}

static void testManyProducerSingleConsumer(int (*fnProducer)(void*), int (*fnConsumer)(void*)) {
  thrd_t tProducer1;
  thrd_t tProducer2;
  thrd_t tConsumer;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer1, fnProducer, queue);
  thrd_create(&tProducer2, fnProducer, queue);
  thrd_create(&tConsumer, fnConsumer, queue);

  thrd_join(tProducer1, NULL);
  thrd_join(tProducer2, NULL);

  char *item = strdup("end");
  while (!queue_add(queue, item)) thrd_yield();

  thrd_join(tConsumer, NULL);

  assert(queue_count(queue) == 0);

  queue_destroy(queue);
}

static void testSingleProducerManyConsumer(int (*fnProducer)(void*), int (*fnConsumer)(void*)) {
  thrd_t tProducer1;
  thrd_t tConsumer1;
  thrd_t tConsumer2;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer1, fnProducer, queue);
  thrd_create(&tConsumer1, fnConsumer, queue);
  thrd_create(&tConsumer2, fnConsumer, queue);

  thrd_join(tProducer1, NULL);

  char *item1 = strdup("end");
  while (!queue_add(queue, item1)) thrd_yield();

  char *item2 = strdup("end");
  while (!queue_add(queue, item2)) thrd_yield();

  thrd_join(tConsumer1, NULL);
  thrd_join(tConsumer2, NULL);

  assert(queue_count(queue) == 0);

  queue_destroy(queue);
}

static void testManyProducerManyConsumer(int (*fnProducer)(void*), int (*fnConsumer)(void*)) {
  thrd_t tProducer1;
  thrd_t tProducer2;
  thrd_t tConsumer1;
  thrd_t tConsumer2;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer1, fnProducer, queue);
  thrd_create(&tProducer2, fnProducer, queue);
  thrd_create(&tConsumer1, fnConsumer, queue);
  thrd_create(&tConsumer2, fnConsumer, queue);

  thrd_join(tProducer1, NULL);
  thrd_join(tProducer2, NULL);

  char *item1 = strdup("end");
  while (!queue_add(queue, item1)) thrd_yield();

  char *item2 = strdup("end");
  while (!queue_add(queue, item2)) thrd_yield();

  thrd_join(tConsumer1, NULL);
  thrd_join(tConsumer2, NULL);

  assert(queue_count(queue) == 0);

  queue_destroy(queue);
}

int main() {
  testCreate();
  testAdd();
  testGet();

  testSingleProducerSingleConsumer(testFastProducer, testFastConsumer);
  testManyProducerSingleConsumer(testFastProducer, testFastConsumer);
  testSingleProducerManyConsumer(testFastProducer, testFastConsumer);
  testManyProducerManyConsumer(testFastProducer, testFastConsumer);

  testSingleProducerSingleConsumer(testFastProducer, testSlowConsumer);
  testSingleProducerSingleConsumer(testSlowProducer, testFastConsumer);

  testManyProducerSingleConsumer(testFastProducer, testSlowConsumer);
  testManyProducerSingleConsumer(testSlowProducer, testFastConsumer);

  testSingleProducerManyConsumer(testFastProducer, testSlowConsumer);
  testSingleProducerManyConsumer(testSlowProducer, testFastConsumer);

  testManyProducerManyConsumer(testFastProducer, testSlowConsumer);
  testManyProducerManyConsumer(testSlowProducer, testFastConsumer);

  return 0;
}
