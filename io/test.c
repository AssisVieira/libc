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

/**
 * Test open and close events.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "io/io.h"

static pthread_t startServer();
static void onServerEvent(void *context, int fd, IOEvent event);
static pthread_t clientConnect();
static void * clientTask(void *arg);
static void * serverTask(void *arg);

static bool eventReadThrowred = false;

int main() {
  void *resClient = NULL;
  void *resServer = NULL;

  pthread_t thServer = startServer();

  pthread_t thClient = clientConnect();

  assert(pthread_join(thServer, &resServer) == 0);
  assert(pthread_join(thClient, &resClient) == 0);

  assert(resClient == NULL);
  assert(resServer == NULL);

  return 0;
}

static void * clientTask(void *arg) {
  int fd = -1;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_family = AF_INET;
  address.sin_port = htons(2000);

  fd = socket(AF_INET, SOCK_STREAM, 0);

  assert(fd >= 0);

  assert(connect(fd, &address, sizeof(address)) == 0);

  assert(close(fd) == 0);

  return NULL;
}

static pthread_t clientConnect() {
  pthread_t thread;

  assert(pthread_create(&thread, NULL, clientTask, NULL) == 0);

  return thread;
}

static void onServerEvent(void *context, int fd, IOEvent event) {
  assert(context == NULL);
  assert(fd >= 0);
  assert((event & IO_READ) != 0);

  eventReadThrowred = true;

  io_close(io_current(), 0);
}

static void * serverTask(void *arg) {
  IO *io = NULL;
  int fd = -1;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_family = AF_INET;
  address.sin_port = htons(2000);

  fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == -1) return NULL;

  int reusePortEnabled = 1;

  assert(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reusePortEnabled,
        sizeof(reusePortEnabled)) == 0);

  assert(bind(fd, &address, sizeof(address)) == 0);

  assert(fcntl(fd, F_SETFL, O_NONBLOCK) == 0);

  assert(listen(fd, 10) == 0);

  io = io_new();

  assert(io_add(io, fd, IO_READ | IO_EDGE_TRIGGERED, NULL, onServerEvent) == 0);

  assert(io_run(io, 10) == 0);

  return NULL;
}

static pthread_t startServer() {
  pthread_t thread;

  assert(pthread_create(&thread, NULL, serverTask, NULL) == 0);

  return thread;
}
