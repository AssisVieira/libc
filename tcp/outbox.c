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

#include "outbox.h"
#include "tcp.h"
#include <sys/socket.h>

////////////////////////////////////////////////////////////////////////////////

static void flush(TcpOutbox *outbox);
static void onReady(void *obj, int fd, IOEventType event);

////////////////////////////////////////////////////////////////////////////////

int tcpOutbox_init(TcpOutbox *outbox, int fd, size_t size, void *ctx,
                   TcpOutboxOnError onError, TcpOutboxOnFlushed onFlushed) {
  outbox->fd = fd;
  outbox->canFlush = true;
  outbox->ioeventInstalled = false;
  outbox->ctx = ctx;
  outbox->onError = onError;
  outbox->onFlushed = onFlushed;

  if (buff_init(&outbox->buff, size)) {
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int tcpOutbox_close(TcpOutbox *outbox) {
  int r = 0;

  if (shutdown(outbox->fd, SHUT_WR)) {
    r = -1;
  }

  buff_free(&outbox->buff);

  return r;
}

////////////////////////////////////////////////////////////////////////////////

void tcpOutbox_flush(TcpOutbox *outbox) {
  if (outbox->canFlush) {
    flush(outbox);
  }
}

////////////////////////////////////////////////////////////////////////////////

BuffWriter *tcpOutbox_writer(TcpOutbox *outbox) {
  return buff_writer(&outbox->buff);
}

////////////////////////////////////////////////////////////////////////////////

static void onReady(void *obj, int fd, IOEventType event) {
  (void)fd;
  (void)event;
  TcpOutbox *outbox = obj;
  outbox->canFlush = true;
  flush(outbox);
}

////////////////////////////////////////////////////////////////////////////////

static void flush(TcpOutbox *outbox) {
  TcpStatus status = tcp_write(outbox->fd, buff_reader(&outbox->buff));

  if (status == TCP_OK) {
    outbox->onFlushed(outbox->ctx, outbox);
  }

  if (status == TCP_TRY_AGAIN) {
    outbox->canFlush = false;

    if (!outbox->ioeventInstalled) {
      int r = ioevent_listen(outbox->fd, IOEVENT_TYPE_WRITE, outbox, onReady);

      if (r != 0) {
        outbox->onError(outbox->ctx, outbox);
        return;
      }

      outbox->ioeventInstalled = true;
    }
  }

  if (status == TCP_ERROR) {
    outbox->onError(outbox->ctx, outbox);
    return;
  }
}
