#include <stdio.h>
#include <stdlib.h>
#include "ring.h"
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
  volatile int reader;
  volatile int writer;
  volatile int count;
  Msg *items[];
};

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

void msg_free(Msg *msg) {
  free(msg);
}

Ring *ring_new(int count) {
  Ring *ring = malloc(sizeof(Ring) + (sizeof(Msg *) * count));

  if (ring == NULL) {
    return NULL;
  }

  ring->reader = -1;
  ring->writer = -1;
  ring->count = count;

  return ring;
}

void ring_free(Ring *ring) {
  free(ring);
}

bool ring_isFull(Ring *ring) {
  if ((ring->reader == ring->writer + 1) || (ring->reader == 0 && ring->writer == ring->count - 1)) return true;
  return false;
}

bool ring_isEmpty(Ring *ring) {
  if (ring->reader == -1) return true;
  return false;
}

void ring_add(Ring *ring, const Msg *item) {
  if (!ring_isFull(ring)) {
    if (ring->reader == -1) ring->reader = 0;
    int i = (ring->writer + 1) % ring->count;
    ring->items[i] = msg_clone(item);
    ring->writer = i;
  }
}

Msg * ring_get(Ring *ring) {
  Msg *item = NULL;
  if (ring_isEmpty(ring)) {
    return NULL;
  } else {
    item = ring->items[ring->reader];
    if (ring->reader == ring->writer) {
      ring->reader = -1;
      ring->writer = -1;
    } else {
      ring->reader = (ring->reader + 1) % ring->count;
    }
    return item;
  }
}

int producer_worker(void *arg) {
  Ring *ring = arg;  
  int count = -1;
  char dados[1000];

  printf("producer started\n");

  while (count < 1000) {
    while (ring_isFull(ring)){ 
      thrd_yield(); 
    }

    snprintf(dados, sizeof(dados), "%d-msg", ++count);

    Msg *msg = msg_new(count, dados, strlen(dados) + 1);

    ring_add(ring, msg);

    msg_free(msg);
  }

  printf("producer exit 0\n");
  return 0;
}

int consumer_worker(void *arg) {
  Ring *ring = arg;  

  printf("consumer started\n");

  while (true) {
    while (ring_isEmpty(ring)) {
      thrd_yield();
    }

    Msg *msg = ring_get(ring);

    printf("%d -> %s\n", msg->type, msg->payload);

    msg_free(msg);
  }

  printf("consumer exit 0\n");

  return 0;
}

int main() {
  Ring *ring = ring_new(1000);

  thrd_t producer;
  thrd_t consumer;
  int resProducer;
  int resConsumer;

  thrd_create(&consumer, consumer_worker, ring);
  thrd_create(&producer, producer_worker, ring);

  thrd_join(producer, &resProducer);
  thrd_join(consumer, &resConsumer);

  if (resProducer || resConsumer) {
    return -1;
  }

  return 0;
}