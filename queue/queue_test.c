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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "asserts/asserts.h"

#define DEBUG

static void testCreate() {
  {
    Queue *queue = queue_create(0);
    assert(queue == NULL);
    assertIntEqual(queue_count(queue), 0);
  }
  {
    Queue *queue = queue_create(1);
    assert(queue != NULL);
    assertIntEqual(queue_count(queue), 0);
    queue_destroy(queue);
  }
}

static void testAdd() {
  Queue *queue = queue_create(3);

  assert(queue_add(queue, "a", true));
  assert(queue_count(queue) == 1);

  assert(queue_add(queue, "b", true));
  assert(queue_count(queue) == 2);

  assert(queue_add(queue, "c", true));
  assert(queue_count(queue) == 3);

  assert(!queue_add(queue, "c", false));
  assert(queue_count(queue) == 3);

  queue_destroy(queue);
}

static void testGet() {
  Queue *queue = queue_create(3);
  assertIntEqual(queue_count(queue), 0);

  assert(queue_add(queue, "a", true));
  assert(queue_count(queue) == 1);

  assert(queue_add(queue, "b", true));
  assert(queue_count(queue) == 2);

  assert(strcmp(queue_get(queue, true), "a") == 0);
  assert(queue_count(queue) == 1);

  assert(strcmp(queue_get(queue, true), "b") == 0);
  assertIntEqual(queue_count(queue), 0);

  assertNull(queue_get(queue, false));

  queue_destroy(queue);
}

static int testFastConsumer(void *arg) {
  Queue *queue = arg;
  char *item = NULL;
#ifdef DEBUG
  size_t count = 0;
#endif

  while (true) {
    item = queue_get(queue, true);

    assert(item != NULL);

    if (strcmp(item, "abc")) break;

    free(item);

#ifdef DEBUG
    fprintf(stdout, "[consumer] (count=%ld)\n", count++);
    fflush(stdout);
#endif
  }

  assert(strcmp(item, "end") == 0);
  free(item);

#ifdef DEBUG
  fprintf(stdout, "[consumer] (count=%ld) END!\n", count++);
  fflush(stdout);
#endif

  return 0;
}

static int testSlowConsumer(void *arg) {
  Queue *queue = arg;
  char *item = NULL;

  while (true) {
    // sleep 0.1ms
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 100000}, NULL);

    item = queue_get(queue, true);

    assert(item != NULL);

    if (strcmp(item, "abc")) break;

    free(item);
  }

  assert(strcmp(item, "end") == 0);
  free(item);

  return 0;
}

static int testSlowProducer(void *arg) {
  Queue *queue = arg;
#ifdef DEBUG
  size_t count = 0;
#endif

  for (int i = 0; i < 10000; i++) {
    // sleep 0.1ms
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 100000}, NULL);

    queue_add(queue, strdup("abc"), true);

#ifdef DEBUG
    fprintf(stdout, "[producer] (count=%ld) i = %d\n", count++, i);
    fflush(stdout);
#endif
  }

#ifdef DEBUG
  fprintf(stdout, "[producer] (count=%ld) END!\n", count++);
  fflush(stdout);
#endif

  return 0;
}

static int testFastProducer(void *arg) {
  Queue *queue = arg;
#ifdef DEBUG
  size_t count = 0;
#endif

  for (int i = 0; i < 10000; i++) {
    queue_add(queue, strdup("abc"), true);
#ifdef DEBUG
    fprintf(stdout, "[producer] (count=%ld) i = %d\n", count++, i);
    fflush(stdout);
#endif
  }

#ifdef DEBUG
  fprintf(stdout, "[producer] (count=%ld) END!\n", count++);
  fflush(stdout);
#endif

  return 0;
}

static void testSingleProducerSingleConsumer(int (*fnProducer)(void *),
                                             int (*fnConsumer)(void *)) {
  thrd_t tProducer;
  thrd_t tConsumer;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer, fnProducer, queue);
  thrd_create(&tConsumer, fnConsumer, queue);

  thrd_join(tProducer, NULL);

  char *item = strdup("end");
  queue_add(queue, item, true);

  thrd_join(tConsumer, NULL);

  assertIntEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testManyProducerSingleConsumer(int (*fnProducer)(void *),
                                           int (*fnConsumer)(void *)) {
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
  queue_add(queue, item, true);

  thrd_join(tConsumer, NULL);

  assertIntEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testSingleProducerManyConsumer(int (*fnProducer)(void *),
                                           int (*fnConsumer)(void *)) {
  thrd_t tProducer1;
  thrd_t tConsumer1;
  thrd_t tConsumer2;
  Queue *queue = queue_create(100);

  thrd_create(&tProducer1, fnProducer, queue);
  thrd_create(&tConsumer1, fnConsumer, queue);
  thrd_create(&tConsumer2, fnConsumer, queue);

  thrd_join(tProducer1, NULL);

  char *item1 = strdup("end");
  queue_add(queue, item1, true);

  char *item2 = strdup("end");
  queue_add(queue, item2, true);

  thrd_join(tConsumer1, NULL);
  thrd_join(tConsumer2, NULL);

  assertIntEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testManyProducerManyConsumer(int (*fnProducer)(void *),
                                         int (*fnConsumer)(void *)) {
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
  queue_add(queue, item1, true);

  char *item2 = strdup("end");
  queue_add(queue, item2, true);

  thrd_join(tConsumer1, NULL);
  thrd_join(tConsumer2, NULL);

  assertIntEqual(queue_count(queue), 0);

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
