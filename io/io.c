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

#include "io.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "log/log.h"

typedef struct IOFd {
  void *context;
  IOListener listener;
  struct epoll_event epollEvent;
} IOFd;

typedef struct IO {
  int epoll;
  // Vetor de struct epollEvent, em que os índices são file descriptors.
  Vetor *fds;
  bool close;
  int closeResult;
} IO;

////////////////////////////////////////////////////////////////////////////////

static void NULL_LISTENER(void *context, int fd, IOEvent events);
static int io_open();

////////////////////////////////////////////////////////////////////////////////

static thread_local IO io = {
    .epoll = -1,
    .fds = NULL,
    .close = true,
};

////////////////////////////////////////////////////////////////////////////////

void io_close(int result) {
  log_dbug("io", "Fechando io...\n");
  io.close = true;
  io.closeResult = result;
}

////////////////////////////////////////////////////////////////////////////////

int io_run(int maxEvents) {
  const int INFINITE_TIME = -1;
  struct epoll_event fds[maxEvents];

  if (io_open()) return -1;

  while (!io.close) {
    log_dbug("io", "Aguardando eventos...\n");

    int numFds = epoll_wait(io.epoll, fds, maxEvents, INFINITE_TIME);

    log_dbug("io", "File descriptors prontos: %d.\n", numFds);

    if (numFds == -1 && errno != EINTR) {
      log_erro("io", "Erro em epoll_wait(): %d - %s.\n", errno,
               strerror(errno));
      io_close(-1);
      continue;
    }

    for (int i = 0; i < numFds; i++) {
      int fd = fds[i].data.fd;
      IOFd *ioFd = vetor_item(io.fds, (size_t)fd);
      ioFd->listener(ioFd->context, fd, fds[i].events);
    }
  }

  if (close(io.epoll) == -1) {
    log_erro("io", "Erro em close(): %d - %s.\n", errno, strerror(errno));
  }

  io.epoll = -1;

  for (int i = 0; i < vetor_qtd(io.fds); i++) {
    free(vetor_item(io.fds, i));
  }

  vetor_destruir(io.fds);

  log_dbug("io", "Fechado.\n");

  return io.closeResult;
}

////////////////////////////////////////////////////////////////////////////////

int io_add(int fd, IOEvent events, void *context, IOListener listener) {
  if (io_open()) return -1;

  IOFd *ioFd = vetor_item(io.fds, (size_t)fd);

  if (ioFd == NULL) {
    ioFd = malloc(sizeof(IOFd));
    vetor_inserir(io.fds, (size_t)fd, ioFd);
  }

  ioFd->epollEvent.events = events;
  ioFd->epollEvent.data.fd = fd;
  ioFd->context = context;
  ioFd->listener = listener;

  log_dbug("io", "Instalando monitor %d.\n", fd);

  return epoll_ctl(io.epoll, EPOLL_CTL_ADD, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

int io_mod(int fd, IOEvent events, void *context, IOListener listener) {
  if (io_open()) return -1;

  IOFd *ioFd = vetor_item(io.fds, (size_t)fd);

  if (ioFd == NULL) return -1;

  ioFd->epollEvent.events = events;
  ioFd->context = context;
  ioFd->listener = listener;

  log_dbug("io", "Modificando monitor: %d.\n", fd);

  return epoll_ctl(io.epoll, EPOLL_CTL_MOD, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

int io_del(int fd) {
  if (io_open()) return -1;

  IOFd *ioFd = vetor_item(io.fds, (size_t)fd);

  if (ioFd == NULL) return -1;

  ioFd->epollEvent.data.fd = -1;
  ioFd->epollEvent.events = 0;
  ioFd->context = NULL;
  ioFd->listener = NULL_LISTENER;

  log_dbug("io", "Removendo monitor: %d.\n", fd);

  return epoll_ctl(io.epoll, EPOLL_CTL_DEL, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

static int io_open() {
  if (!io.close) return 0;

  log_dbug("io", "Abrindo io...\n");

  io.epoll = epoll_create1(0);

  if (io.epoll <= 0) {
    log_erro("io", "Erro em epoll_create1(): %d - %s.\n", errno,
             strerror(errno));
    return -1;
  }

  io.fds = vetor_criar(10);

  if (io.fds == NULL) {
    log_erro("io", "vetor_criar()\n");
    close(io.epoll);
    io.epoll = -1;
    return -1;
  }

  io.close = false;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void NULL_LISTENER(void *context, int fd, IOEvent events) {
  (void)context;
  (void)fd;
  (void)events;
}