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
#include <pthread.h>
#include <unistd.h>

#include "asserts/asserts.h"

#define DEBUG

static void testCreate() {
  {
    Queue *queue = queue_create(0);
    assert(queue == NULL);
    assertEqual(queue_count(queue), 0);
  }
  {
    Queue *queue = queue_create(1);
    assert(queue != NULL);
    assertEqual(queue_count(queue), 0);
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
  assertEqual(queue_count(queue), 0);

  assert(queue_add(queue, "a", true));
  assert(queue_count(queue) == 1);

  assert(queue_add(queue, "b", true));
  assert(queue_count(queue) == 2);

  assert(strcmp(queue_get(queue, true), "a") == 0);
  assert(queue_count(queue) == 1);

  assert(strcmp(queue_get(queue, true), "b") == 0);
  assertEqual(queue_count(queue), 0);

  assertNull(queue_get(queue, false));

  queue_destroy(queue);
}

static void * testFastConsumer(void *arg) {
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

  return NULL;
}

static void * testSlowConsumer(void *arg) {
  Queue *queue = arg;
  char *item = NULL;

  while (true) {
    usleep(1000);

    item = queue_get(queue, true);

    assert(item != NULL);

    if (strcmp(item, "abc")) break;

    free(item);
  }

  assert(strcmp(item, "end") == 0);
  free(item);

  return NULL;
}

static void * testSlowProducer(void *arg) {
  Queue *queue = arg;
#ifdef DEBUG
  size_t count = 0;
#endif

  for (int i = 0; i < 10000; i++) {
    usleep(1000);

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

  return NULL;
}

static void * testFastProducer(void *arg) {
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

  return NULL;
}

static void testSingleProducerSingleConsumer(void *(*fnProducer)(void *),
                                             void *(*fnConsumer)(void *)) {
  pthread_t tProducer;
  pthread_t tConsumer;
  Queue *queue = queue_create(100);

  pthread_create(&tProducer, NULL, fnProducer, queue);
  pthread_create(&tConsumer, NULL, fnConsumer, queue);

  pthread_join(tProducer, NULL);

  char *item = strdup("end");
  queue_add(queue, item, true);

  pthread_join(tConsumer, NULL);

  assertEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testManyProducerSingleConsumer(void *(*fnProducer)(void *),
                                           void *(*fnConsumer)(void *)) {
  pthread_t tProducer1;
  pthread_t tProducer2;
  pthread_t tConsumer;
  Queue *queue = queue_create(100);

  pthread_create(&tProducer1, NULL, fnProducer, queue);
  pthread_create(&tProducer2, NULL, fnProducer, queue);
  pthread_create(&tConsumer, NULL, fnConsumer, queue);

  pthread_join(tProducer1, NULL);
  pthread_join(tProducer2, NULL);

  char *item = strdup("end");
  queue_add(queue, item, true);

  pthread_join(tConsumer, NULL);

  assertEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testSingleProducerManyConsumer(void *(*fnProducer)(void *),
                                           void *(*fnConsumer)(void *)) {
  pthread_t tProducer1;
  pthread_t tConsumer1;
  pthread_t tConsumer2;
  Queue *queue = queue_create(100);

  pthread_create(&tProducer1, NULL, fnProducer, queue);
  pthread_create(&tConsumer1, NULL, fnConsumer, queue);
  pthread_create(&tConsumer2, NULL, fnConsumer, queue);

  pthread_join(tProducer1, NULL);

  char *item1 = strdup("end");
  queue_add(queue, item1, true);

  char *item2 = strdup("end");
  queue_add(queue, item2, true);

  pthread_join(tConsumer1, NULL);
  pthread_join(tConsumer2, NULL);

  assertEqual(queue_count(queue), 0);

  queue_destroy(queue);
}

static void testManyProducerManyConsumer(void *(*fnProducer)(void *),
                                         void *(*fnConsumer)(void *)) {
  pthread_t tProducer1;
  pthread_t tProducer2;
  pthread_t tConsumer1;
  pthread_t tConsumer2;
  Queue *queue = queue_create(100);

  pthread_create(&tProducer1, NULL, fnProducer, queue);
  pthread_create(&tProducer2, NULL, fnProducer, queue);
  pthread_create(&tConsumer1, NULL, fnConsumer, queue);
  pthread_create(&tConsumer2, NULL, fnConsumer, queue);

  pthread_join(tProducer1, NULL);
  pthread_join(tProducer2, NULL);

  char *item1 = strdup("end");
  queue_add(queue, item1, true);

  char *item2 = strdup("end");
  queue_add(queue, item2, true);

  pthread_join(tConsumer1, NULL);
  pthread_join(tConsumer2, NULL);

  assertEqual(queue_count(queue), 0);

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
