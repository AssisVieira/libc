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

#include "ioevent/ioevent.h"
#include "outbox.h"
#include "port.h"
#include <string.h>
#include <unistd.h>

static void onNewClient(TcpPort *port, int fd);
static void onOutboxError(void *context, TcpOutbox *outbox);
static void onOutboxFlushed(void *context, TcpOutbox *outbox);

static bool finish = false;

int main() {
  TcpPort port;

  ioevent_open();

  if (tcpPort_open(&port, "5000", 100, onNewClient))
    return -1;

  ioevent_run(&finish);

  tcpPort_close(&port);

  return 0;
}

static void onNewClient(TcpPort *port, int fd) {
  TcpOutbox outbox;

  if (tcpOutbox_init(&outbox, fd, 10000, NULL, onOutboxError,
                     onOutboxFlushed)) {
    return;
  }

  BuffWriter *writer = tcpOutbox_writer(&outbox);

  buff_writer_write(writer, "00000000", strlen("00000000"));

  for (int i = 0; i < 1000; i++) {
    buff_writer_write(writer, "blablablablbalalabl blabalbala",
                      strlen("blablablablbalalabl blabalbala"));

    tcpOutbox_flush(&outbox);

    // if (i % 100 == 0) {
    //   sleep(1);
    // }
  }

  buff_writer_write(writer, "00000000", strlen("00000000"));

  tcpOutbox_flush(&outbox);

  tcpOutbox_close(&outbox);

  close(fd);
  tcpPort_onClientDisconnected(port);

  // finish = true;
}

static void onOutboxError(void *context, TcpOutbox *outbox) {}
static void onOutboxFlushed(void *context, TcpOutbox *outbox) {}
