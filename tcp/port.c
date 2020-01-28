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

#include "port.h"
#include "log/log.h"
#include "tcp.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

static void onNewClient(void *obj, int fd, IOEventType e);
static void acceptClients(TcpPort *port);

////////////////////////////////////////////////////////////////////////////////

int tcpPort_open(TcpPort *port, const char *num, size_t maxClients,
                 TcpPortOnConnected onConnected) {
  port->onConnected = onConnected;
  port->port = num;
  port->maxClients = maxClients;
  port->numClients = 0;
  port->clientPending = false;

  port->fd = tcp_listen(port->port, 128);

  if (port->fd == -1) {
    return -1;
  }

  if (ioevent_install(port->fd, true)) {
    return -1;
  }

  if (ioevent_listen(port->fd, IOEVENT_TYPE_READ, port, onNewClient)) {
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void tcpPort_onClientDisconnected(TcpPort *port) {
  port->numClients--;
  acceptClients(port);
}

////////////////////////////////////////////////////////////////////////////////

int tcpPort_close(TcpPort *port) {
  // Ao fechar a conexão, automaticamente ela é removida do ioevent.
  if (tcp_close(port->fd) != 0)
    return -1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void onNewClient(void *obj, int fd, IOEventType e) {
  (void)fd;
  TcpPort *port = obj;
  port->clientPending = true;
  acceptClients(port);
}

////////////////////////////////////////////////////////////////////////////////

static void acceptClients(TcpPort *port) {
  while (port->clientPending && port->numClients < port->maxClients) {
    int fd = tcp_accept(port->fd);

    if (fd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        port->clientPending = false;
        log_dbug("tcp-port", "Tentar depois.\n");
        break;
      } else {
        log_erro("tcp-port", "tcp_accept(): %d - %s\n", errno, strerror(errno));
        return;
      }
    }

    port->numClients++;

    log_dbug("tcp-port", "Conexão aceita [%u/%u]: %d.\n", port->numClients,
             port->maxClients, fd);

    port->onConnected(port, fd);
  }
}
