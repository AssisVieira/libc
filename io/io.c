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

static _Thread_local IO *CURRENT = NULL;

////////////////////////////////////////////////////////////////////////////////

static void NULL_LISTENER(void *context, int fd, IOEvent events);

////////////////////////////////////////////////////////////////////////////////

IO *io_current() { return CURRENT; }

////////////////////////////////////////////////////////////////////////////////

void io_close(IO *io, int result) {
  log_dbug("io", "Closing io: %d\n", io->epoll);
  io->close = true;
  io->closeResult = result;
}

////////////////////////////////////////////////////////////////////////////////

int io_run(IO *io, int maxEvents) {
  const int INFINITE_TIME = -1;
  struct epoll_event fds[maxEvents];

  if (io == NULL) {
    io = io_new();
    if (io == NULL) {
      log_erro("io", "io_new()\n");
      return -1;
    }
  }

  CURRENT = io;

  log_dbug("io", "Running io: %d, maxEvents: %d\n", io->epoll, maxEvents);

  while (!io->close) {
    log_dbug("io", "Waiting events: %d\n", io->epoll);

    int numFds = epoll_wait(io->epoll, fds, maxEvents, INFINITE_TIME);

    log_dbug("io", "File descriptors ready: %d.\n", numFds);

    if (numFds == -1 && errno != EINTR) {
      log_erro("io", "epoll_wait(): %d - %s.\n", errno, strerror(errno));
      io_close(io, -1);
      continue;
    }

    for (int i = 0; i < numFds; i++) {
      int fd = fds[i].data.fd;
      IOFd *ioFd = vetor_item(io->fds, (size_t)fd);
      ioFd->listener(ioFd->context, fd, fds[i].events);
    }
  }

  if (close(io->epoll) == -1) {
    log_erro("io", "close(): %d - %s.\n", errno, strerror(errno));
  }

  io->epoll = -1;

  for (int i = 0; i < vetor_qtd(io->fds); i++) {
    free(vetor_item(io->fds, i));
  }

  vetor_destruir(io->fds);

  log_dbug("io", "IO closed: %d.\n", io->epoll);
  int result = io->closeResult;

  free(io);

  return result;
}

////////////////////////////////////////////////////////////////////////////////

int io_add(IO *io, int fd, IOEvent events, void *context, IOListener listener) {
  IOFd *ioFd = vetor_item(io->fds, (size_t)fd);

  if (ioFd == NULL) {
    ioFd = malloc(sizeof(IOFd));
    vetor_inserir(io->fds, (size_t)fd, ioFd);
  }

  ioFd->epollEvent.events = events;
  ioFd->epollEvent.data.fd = fd;
  ioFd->context = context;
  ioFd->listener = listener;

  log_dbug("io", "Instalando monitor %d.\n", fd);

  return epoll_ctl(io->epoll, EPOLL_CTL_ADD, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

int io_mod(IO *io, int fd, IOEvent events, void *context, IOListener listener) {
  IOFd *ioFd = vetor_item(io->fds, (size_t)fd);

  if (ioFd == NULL) return -1;

  ioFd->epollEvent.events = events;
  ioFd->context = context;
  ioFd->listener = listener;

  log_dbug("io", "Modificando monitor: %d.\n", fd);

  return epoll_ctl(io->epoll, EPOLL_CTL_MOD, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

int io_del(IO *io, int fd) {
  IOFd *ioFd = vetor_item(io->fds, (size_t)fd);

  if (ioFd == NULL) return -1;

  ioFd->epollEvent.data.fd = -1;
  ioFd->epollEvent.events = 0;
  ioFd->context = NULL;
  ioFd->listener = NULL_LISTENER;

  log_dbug("io", "Removendo monitor: %d.\n", fd);

  return epoll_ctl(io->epoll, EPOLL_CTL_DEL, fd, &ioFd->epollEvent);
}

////////////////////////////////////////////////////////////////////////////////

IO *io_new() {
  IO *io = malloc(sizeof(IO));

  if (io == NULL) {
    log_erro("io", "malloc(): %d - %s.\n", errno, strerror(errno));
    return NULL;
  }

  io->epoll = epoll_create1(0);

  if (io->epoll <= 0) {
    log_erro("io", "epoll_create1(): %d - %s.\n", errno, strerror(errno));
    free(io);
    return NULL;
  }

  io->fds = vetor_criar(10);

  if (io->fds == NULL) {
    log_erro("io", "vetor_criar()\n");
    close(io->epoll);
    free(io);
    return NULL;
  }

  io->close = false;

  CURRENT = io;

  log_dbug("io", "IO created: %d.\n", io->epoll);

  return io;
}

////////////////////////////////////////////////////////////////////////////////

static void NULL_LISTENER(void *context, int fd, IOEvent events) {
  (void)context;
  (void)fd;
  (void)events;
}
