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
 
#ifndef TCP_OUTBOX_H
#define TCP_OUTBOX_H

#include "buff/buff.h"
#include "ioevent/ioevent.h"
#include <stdbool.h>

typedef struct TcpOutbox TcpOutbox;

typedef void (*TcpOutboxOnError)(void *ctx, TcpOutbox *outbox);
typedef void (*TcpOutboxOnFlushed)(void *ctx, TcpOutbox *outbox);

typedef struct TcpOutbox {
  int fd;
  Buff buff;
  bool canFlush;
  bool ioeventInstalled;
  void *ctx;
  TcpOutboxOnError onError;
  TcpOutboxOnFlushed onFlushed;
} TcpOutbox;

int tcpOutbox_init(TcpOutbox *outbox, int fd, size_t size, void *ctx,
                   TcpOutboxOnError onError, TcpOutboxOnFlushed onFlushed);

int tcpOutbox_close(TcpOutbox *outbox);

void tcpOutbox_flush(TcpOutbox *outbox);

BuffWriter *tcpOutbox_writer(TcpOutbox *outbox);

#endif
