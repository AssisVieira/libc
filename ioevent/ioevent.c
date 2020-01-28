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

#include "ioevent.h"
#include "log/log.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

typedef struct IOEventListener {
  void *context;
  IOEventListenerFn func;
} IOEventListener;

typedef struct IOEventFd {
  // Cada índice deste vetor é um tipo de eventType.
  IOEventListener listeners[IOEVENT_TYPE_MAX];
  struct epoll_event epollEvent;
} IOEventFd;

typedef struct IOEvent {
  int epoll;
  // Vetor de Fdes, em que os índices são descritores.
  Vetor fds;
  bool close;
  int closeResult;
} IOEvent;

////////////////////////////////////////////////////////////////////////////////

static uint32_t EPOLL_EVENTS[IOEVENT_TYPE_MAX];

static void NULL_LISTENER(void *context, int fd, IOEventType eventType);

////////////////////////////////////////////////////////////////////////////////

static thread_local IOEvent ioevent = {
    .epoll = -1,
    .close = true,
};

////////////////////////////////////////////////////////////////////////////////

int ioevent_open() {
  log_dbug("ioevent", "Criando ioevent...\n");

  EPOLL_EVENTS[IOEVENT_TYPE_READ] = EPOLLIN;
  EPOLL_EVENTS[IOEVENT_TYPE_WRITE] = EPOLLOUT;
  EPOLL_EVENTS[IOEVENT_TYPE_DISCONNECTED] = EPOLLHUP;
  EPOLL_EVENTS[IOEVENT_TYPE_ERROR] = EPOLLERR;

  ioevent.epoll = epoll_create1(0);

  if (ioevent.epoll == -1) {
    log_erro("ioevent", "Erro em epoll_create1(): %d - %s.\n", errno,
             strerror(errno));
    return -1;
  }

  if (vetor_init(&ioevent.fds, 10) != 0) {
    log_erro("ioevent", "Erro na criação do ioevent.\n");
    close(ioevent.epoll);
    ioevent.epoll = -1;
    return -1;
  }

  ioevent.close = false;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int ioevent_close(int result) {
  log_dbug("ioevent", "Fechando ioevent...\n");
  ioevent.close = true;
  ioevent.closeResult = result;
  // TODO: Talvez seja necessário enviar uma notificação pro loop sair da
  // espera, detectar a mudança de estado do ioevent e assim encerrar a execução
  // do ioevent.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int ioevent_run() {
  const int INFINITE_TIME = -1;
  struct epoll_event eventTypes[IOEVENT_EVENTS_MAX];

  log_dbug("ioevent", "Iniciando laço de eventTypes...\n");

  while (!ioevent.close) {
    log_dbug("ioevent", "Aguardando eventos...\n");

    int numFds = epoll_wait(ioevent.epoll, eventTypes, IOEVENT_EVENTS_MAX,
                            INFINITE_TIME);

    log_dbug("ioevent", "File descriptors prontos: %d.\n", numFds);

    if (numFds == -1 && errno != EINTR) {
      log_erro("ioevent", "Erro em epoll_wait(): %d - %s.\n", errno,
               strerror(errno));
      ioevent_close(-1);
      continue;
    }

    for (int i = 0; i < numFds; i++) {
      int fd = eventTypes[i].data.fd;
      IOEventFd *ioeventFd = vetor_item(&ioevent.fds, (size_t)fd);

      if (eventTypes[i].events & EPOLLIN) {
        log_dbug("ioevent", "FD %d - READ.\n", fd);
        IOEventListener *listener = &ioeventFd->listeners[IOEVENT_TYPE_READ];
        listener->func(listener->context, fd, IOEVENT_TYPE_READ);
      }

      if (eventTypes[i].events & EPOLLOUT) {
        log_dbug("ioevent", "FD %d - WRITE.\n", fd);
        IOEventListener *listener = &ioeventFd->listeners[IOEVENT_TYPE_WRITE];
        listener->func(listener->context, fd, IOEVENT_TYPE_WRITE);
      }

      if (eventTypes[i].events & EPOLLHUP) {
        log_dbug("ioevent", "FD %d - HUP.\n", fd);
        IOEventListener *listener =
            &ioeventFd->listeners[IOEVENT_TYPE_DISCONNECTED];
        listener->func(listener->context, fd, IOEVENT_TYPE_DISCONNECTED);
      }

      if (eventTypes[i].events & EPOLLERR) {
        log_dbug("ioevent", "FD %d - ERROR.\n", fd);
        IOEventListener *listener = &ioeventFd->listeners[IOEVENT_TYPE_ERROR];
        listener->func(listener->context, fd, IOEVENT_TYPE_ERROR);
      }
    }
  }

  if (close(ioevent.epoll) == -1) {
    log_erro("ioevent", "Erro em close(): %d - %s.\n", errno, strerror(errno));
  }

  ioevent.epoll = -1;

  for (int i = 0; i < vetor_qtd(&ioevent.fds); i++) {
    free(vetor_item(&ioevent.fds, i));
  }

  vetor_destruir(&ioevent.fds);

  return ioevent.closeResult;
}

////////////////////////////////////////////////////////////////////////////////

int ioevent_install(int fd, bool edgeTriggered) {
  IOEventFd *desc = vetor_item(&ioevent.fds, (size_t)fd);

  log_dbug("ioevent", "Instalando conexão %d.\n", fd);

  if (desc == NULL) {
    desc = malloc(sizeof(IOEventFd));
    vetor_inserir(&ioevent.fds, (size_t)fd, desc);
  }

  desc->listeners[IOEVENT_TYPE_READ].func = NULL_LISTENER;
  desc->listeners[IOEVENT_TYPE_WRITE].func = NULL_LISTENER;
  desc->listeners[IOEVENT_TYPE_DISCONNECTED].func = NULL_LISTENER;
  desc->listeners[IOEVENT_TYPE_ERROR].func = NULL_LISTENER;

  desc->epollEvent.data.fd = fd;
  desc->epollEvent.events = 0;

  if (edgeTriggered) {
    desc->epollEvent.events |= EPOLLET;
  }

  log_dbug("ioevent", "Instalando a conexão %d.\n", fd);

  if (epoll_ctl(ioevent.epoll, EPOLL_CTL_ADD, fd, &desc->epollEvent) != 0) {
    log_erro("ioevent", "Erro em epoll_ctl(): %d - %s.\n", errno,
             strerror(errno));
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int ioevent_listen(int fd, IOEventType eventType, void *context,
                   IOEventListenerFn func) {
  IOEventFd *desc = vetor_item(&ioevent.fds, (size_t)fd);
  desc->listeners[eventType].context = context;
  desc->listeners[eventType].func = func;
  desc->epollEvent.events |= EPOLL_EVENTS[eventType];

  log_dbug("ioevent", "Instalando eventType %02x para conexão %d.\n",
           EPOLL_EVENTS[eventType], fd);

  if (epoll_ctl(ioevent.epoll, EPOLL_CTL_MOD, fd, &desc->epollEvent) != 0) {
    log_erro("ioevent", "Erro em epoll_ctl(): %d - %s.\n", errno,
             strerror(errno));
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int ioevent_nolisten(int fd, IOEventType eventType) {
  IOEventFd *desc = vetor_item(&ioevent.fds, (size_t)fd);
  desc->listeners[eventType].func = NULL_LISTENER;
  desc->epollEvent.events &= ~EPOLL_EVENTS[eventType];

  log_dbug("ioevent", "Removendo eventType %02x para conexão %d.\n",
           EPOLL_EVENTS[eventType], fd);

  if (epoll_ctl(ioevent.epoll, EPOLL_CTL_MOD, fd, &desc->epollEvent) != 0) {
    log_erro("ioevent", "Erro em epoll_ctl(): %d - %s.\n", errno,
             strerror(errno));
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void NULL_LISTENER(void *context, int fd, IOEventType eventType) {
  (void)context;
  (void)fd;
  (void)eventType;
}
