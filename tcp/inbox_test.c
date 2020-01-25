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
 
#include "inbox.h"
#include "log/log.h"
#include "port.h"
#include "tcp.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static void onNewClient(void *objListener, int fd);
static void onInboxNewData(void *context, TcpInbox *inbox);
static void onInboxError(void *context, TcpInbox *inbox);

static TcpInbox inbox;

int main() {
  bool finish = false;

  if (ioevent_init(true)) {
    perror("Error: ioevent_init()\n");
    return -1;
  }

  TcpPort port;

  if (tcpPort_init(&port, "9000", 1, NULL, onNewClient)) {
    return -1;
  }

  ioevent_run(&finish);

  tcpPort_close(&port);

  ioevent_close();

  return 0;
}

static void onNewClient(void *objListener, int fd) {
  printf("TRACE - onNewClient()\n");

  int initRes =
      tcpInbox_init(&inbox, fd, 1000, NULL, onInboxNewData, onInboxError);

  assert(initRes == 0);
}

static void onInboxNewData(void *context, TcpInbox *inbox) {
  printf("TRACE - onInboxNewData()\n");
}

static void onInboxError(void *context, TcpInbox *inbox) {
  printf("TRACE - onInboxNewData()\n");
}
