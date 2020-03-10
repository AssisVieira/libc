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
#include <threads.h>
#include <unistd.h>
#include <assert.h>

#include "io/io.h"

static thrd_t startServer();
static void onServerEvent(void *context, int fd, IOEvent event);
static thrd_t clientConnect();
static int clientTask(void *arg);
static int serverTask(void *arg);

static bool eventReadThrowred = false;

int main() {
  int resClient;
  int resServer;

  thrd_t thServer = startServer();

  thrd_t thClient = clientConnect();

  assert(thrd_join(thServer, &resServer) == 0);
  assert(thrd_join(thClient, &resClient) == 0);

  assert(resClient == 0);
  assert(resServer == 0);

  return 0;
}

static int clientTask(void *arg) {
  int fd = -1;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_family = AF_INET;
  address.sin_port = htons(2000);

  fd = socket(AF_INET, SOCK_STREAM, 0);

  assert(fd >= 0);

  assert(connect(fd, &address, sizeof(address)) == 0);

  assert(close(fd) == 0);

  return 0;
}

static thrd_t clientConnect() {
  thrd_t thread;

  assert(thrd_create(&thread, clientTask, NULL) == 0);

  return thread;
}

static void onServerEvent(void *context, int fd, IOEvent event) {
  assert(context == NULL);
  assert(fd >= 0);
  assert((event & IO_READ) != 0);

  eventReadThrowred = true;

  io_close(io_current(), 0);
}

static int serverTask(void *arg) {
  IO *io = NULL;
  int fd = -1;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_family = AF_INET;
  address.sin_port = htons(2000);

  fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == -1) return -1;

  int reusePortEnabled = 1;

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reusePortEnabled,
                 sizeof(reusePortEnabled))) {
    close(fd);
    return -1;
  }

  if (bind(fd, &address, sizeof(address))) {
    close(fd);
    return -1;
  }

  if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
    close(fd);
    return -1;
  }

  if (listen(fd, 10)) {
    close(fd);
    return -1;
  }

  io = io_new();

  assert(io_add(io, fd, IO_READ | IO_EDGE_TRIGGERED, NULL, onServerEvent) == 0);

  return io_run(io, 10);
}

static thrd_t startServer() {
  thrd_t thread;

  assert(thrd_create(&thread, serverTask, NULL) == 0);

  return thread;
}