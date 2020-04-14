#include <stdio.h>

#include "worker.h"

typedef enum MsgType {
  MSG_INIT,
  MSG_ADD,
  MSG_SUB,
  MSG_RES,
  MSG_QUIT,
} MsgType;

typedef struct MyContext {
  int res;
} MyContext;

typedef struct Msg {
  MsgType type;
  int num;
} Msg;

static WorkerState actor_on_message(void *context, const void *arg);
static WorkerState actor_on_signal(void *context, const void *arg);

int main() {
  Worker *worker = worker_create("Worker 1", 
      actor_on_signal, 
      actor_on_message, 
      sizeof(MyContext));

  Msg msg0 = {.type = MSG_INIT};
  worker_send(worker, &msg0, sizeof(msg0));

  Msg msg1 = {.type = MSG_ADD, .num = 4};
  worker_send(worker, &msg1, sizeof(msg1));

  Msg msg2 = {.type = MSG_SUB, .num = 3};
  worker_send(worker, &msg2, sizeof(msg2));

  Msg msg3 = {.type = MSG_RES};
  worker_send(worker, &msg3, sizeof(msg3));

  Msg msg4 = {.type = MSG_QUIT};
  worker_send(worker, &msg4, sizeof(msg4));

  worker_await(worker);

  printf("Bye\n");

  return 0;
}

static WorkerState actor_on_signal(void *context, const void *msg) {
  if (msg == &Init) {
    
  }
  if (msg == &Close) {

  }
}

static WorkerState actor_on_message(void *context, const void *arg) {
  MyContext *ctx = context;
  const Msg *msg = arg;

  switch (msg->type) {
    case MSG_INIT:
      ctx->res = 0;
      return actor_on_message;
    case MSG_ADD:
      printf("ADD: %d\n", msg->num);
      ctx->res += msg->num;
      return actor_on_message;
    case MSG_SUB:
      printf("SUB: %d\n", msg->num);
      ctx->res -= msg->num;
      return actor_on_message;
    case MSG_QUIT:
      printf("QUIT: %d\n", ctx->res);
      return worker_state_stop;
    default:
      return actor_on_message;
  }
}


