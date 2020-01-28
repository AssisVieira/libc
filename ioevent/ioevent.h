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

#ifndef IOEVENT_H
#define IOEVENT_H

#include "vetor/vetor.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>

#define IOEVENT_EVENTS_MAX 64

typedef enum IOEventType {
  IOEVENT_TYPE_READ = 0,
  IOEVENT_TYPE_WRITE,
  IOEVENT_TYPE_ERROR,
  IOEVENT_TYPE_DISCONNECTED,

  IOEVENT_TYPE_MAX,
} IOEventType;

typedef void (*IOEventListenerFn)(void *context, int fd, IOEventType eventType);

int ioevent_open();

int ioevent_install(int fd, bool edgeTriggered);

int ioevent_listen(int fd, IOEventType eventType, void *context,
                   IOEventListenerFn fnListener);

int ioevent_nolisten(int fd, IOEventType eventType);

int ioevent_run();

int ioevent_close(int result);

#endif
