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
 * Este módulo extende as funcionalidades do epoll, ao permitir a instalação
 * de ouvintes para cada file descriptor monitorado. Além disso, simplifica
 * o uso do epoll, restringindo que cada thread tenha no máximo uma
 * instância de epoll.
 */

#ifndef IO_H
#define IO_H

#include <sys/epoll.h>

#include "vetor/vetor.h"

typedef enum IOEvent {
  IO_READ = EPOLLIN,
  IO_WRITE = EPOLLOUT,
  IO_READ_CLOSED = EPOLLRDHUP,
  IO_PRI = EPOLLPRI,
  IO_EDGE_TRIGGERED = EPOLLET,
  IO_ERROR = EPOLLERR,
  IO_CLOSED = EPOLLHUP,
  IO_ONE_SHOT = EPOLLONESHOT,
  IO_WAKE_UP = EPOLLWAKEUP,
  IO_EXCLUSIVE = EPOLLEXCLUSIVE,
} IOEvent;

typedef void (*IOListener)(void *context, int fd, IOEvent event);

int io_add(int fd, IOEvent events, void *context, IOListener listener);

int io_mod(int fd, IOEvent events, void *context, IOListener listener);

int io_del(int fd);

int io_run(int maxEvents);

void io_close(int result);

#endif
