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
 
#ifndef TCP_PORT_H
#define TCP_PORT_H

#include "ioevent/ioevent.h"
#include <stdbool.h>

typedef struct TcpPort TcpPort;

typedef void (*TcpPortOnConnected)(TcpPort *port, int fd);

typedef struct TcpPort {
  const char *port;
  int fd;
  size_t maxClients;
  size_t numClients;
  bool clientPending;
  TcpPortOnConnected onConnected;
} TcpPort;

/**
 * Abre uma port TCP/IP.
 *
 * Após a abertura, a porta estará pronta para aceitar conexões.
 */
int tcpPort_open(TcpPort *port, const char *num, size_t maxClients,
                 TcpPortOnConnected onConnected);

/**
 * Fecha e destroi a port.
 *
 * Isso não implica no encerramento das conexões abertas pela port.
 *
 * @param  port [description]
 * @return       [description]
 */
int tcpPort_close(TcpPort *port);

/**
 * Notifica a port de que uma conexão foi destruída.
 *
 * Ao saber disso, a port pode liberar acesso a outros pedidos de conexões,
 * caso o número de conexões tenha atingido o limite máximo.
 *
 * @param  port port na qual a conexão foi originalmente criada.
 */
void tcpPort_onClientDisconnected(TcpPort *port);

#endif
