#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <limits.h>
#include <threads.h>
#include <assert.h>
#include <string.h>

typedef struct Msg {
  int type;
  size_t size;
  char payload[];
} Msg;

struct Ring {
  int volatile reader;
  int volatile writer;
  mtx_t mtxReader;
  int max;
  Msg *items[];
};

typedef struct Worker {
  int id;
  thrd_t thread;
  Ring *ring;
} Worker;

typedef struct Producer {
  int id;
  thrd_t thread;
  Ring *ring;
  int totalMsgs;
} Producer;

Msg *msg_new(int type, const char *payload, size_t size) {
  Msg *msg = malloc(sizeof(Msg) + size);
  msg->type = type;
  msg->size = size;
  memcpy(msg->payload, payload, size);
  return msg;
}

Msg *msg_clone(const Msg *orig) {
  Msg *msg = malloc(sizeof(Msg) + orig->size);
  memcpy(msg, orig, sizeof(Msg) + orig->size);
  return msg;
}

void msg_free(Msg *msg) { free(msg); }

Ring *ring_new(int max) {
  Ring *ring = malloc(sizeof(Ring) + (sizeof(Msg *) * max));

  if (ring == NULL) {
    return NULL;
  }

  ring->reader = 0;
  ring->writer = 0;
  ring->max = max;
  mtx_init(&ring->mtxReader, mtx_plain);

  return ring;
}

void ring_free(Ring *ring) { free(ring); }

bool ring_add(Ring *ring, const Msg *item) {
  bool r = false;

  int newWriter = (ring->writer + 1) % ring->max;

  if (newWriter != ring->reader) {
    ring->items[ring->writer] = msg_clone(item);
    ring->writer = newWriter;
    r = true;
  }

  return r;
}

Msg *ring_get(Ring *ring) {
  Msg *msg = NULL;
  mtx_lock(&ring->mtxReader);
  if (ring->reader != ring->writer) {
    msg = ring->items[ring->reader];
    ring->reader = (ring->reader + 1) % ring->max;
  }
  mtx_unlock(&ring->mtxReader);
  return msg;
}

int producer_worker(void *arg) {
  Producer *producer = arg;
  int count = 0;
  char dados[1000];

  printf("producer started\n");

  while (count < producer->totalMsgs) {
    snprintf(dados, sizeof(dados), "%d-msg", count);

    Msg *msg = msg_new(count, dados, strlen(dados) + 1);

    while (!ring_add(producer->ring, msg)) {
      thrd_yield();
    }

    msg_free(msg);

    ++count;
  }

  Msg *msg = msg_new(-1, dados, strlen(dados) + 1);
  count = 0;
  while (count < 100) {
    while (!ring_add(producer->ring, msg)) {
      thrd_yield();
    }
    ++count;
  }
  msg_free(msg);

  printf("producer exit 0\n");
  return 0;
}

int consumer_worker(void *arg) {
  Worker *worker = arg;
  int type = 0;

  printf("consumer started: %d\n", worker->id);
  clock_t begin = clock();

  while (type >= 0) {
    Msg *msg;

    while ((msg = ring_get(worker->ring)) == NULL) {
      thrd_yield();
    }

    type = msg->type;

    printf("%d -> %s\n", msg->type, msg->payload);

    msg_free(msg);
  }

  clock_t end = clock();
  double timeElapsed = ((double)(end - begin) / CLOCKS_PER_SEC) * 1000;
  printf("consumer exit: %d - %.2f ms\n", worker->id, timeElapsed);

  return 0;
}

int main(int argc, char *argsv[]) {
  Ring *ring = ring_new(1000000);
  const int workersCount = atoi(argsv[1]);
  Worker worker[workersCount];
  Producer producer;

  producer.id = 0;
  producer.totalMsgs = atoi(argsv[2]);
  producer.ring = ring;
  producer.thread = 0;

  printf("Workers: %d\n", workersCount);
  printf("Total Msgs: %d\n", producer.totalMsgs);
  printf("Press any key to continue...\n");
  getchar();

  for (int i = 0; i < workersCount; i++) {
    worker[i].id = i;
    worker[i].ring = ring;
  }

  clock_t begin = clock();

  thrd_create(&producer.thread, producer_worker, &producer);
  for (int i = 0; i < workersCount; i++) {
    thrd_create(&worker[i].thread, consumer_worker, &worker[i]);
  }

  thrd_join(producer.thread, NULL);
  for (int i = 0; i < workersCount; i++) {
    thrd_join(worker[i].thread, NULL);
  }

  clock_t end = clock();
  double timeElapsed = ((double)(end - begin) / CLOCKS_PER_SEC) * 1000;
  printf("----------------------\n");
  printf("Time elapsed: %.2f ms\n", timeElapsed);
  printf("Msgs by seg: %.2f\n", ring->max / (timeElapsed / 1000.0) );

  return 0;
}