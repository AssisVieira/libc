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

#ifndef TCP_INBOX_H
#define TCP_INBOX_H

#include "buff/buff.h"
#include "ioevent/ioevent.h"
#include <stdbool.h>

typedef struct TcpInbox TcpInbox;

typedef enum TcpInboxEvent {
  TCPINBOX_EVENT_DATA,
  TCPINBOX_EVENT_ERROR,
} TcpInboxEvent;

typedef void (*TcpInboxOnData)(void *ctx, TcpInbox *inbox);
typedef void (*TcpInboxOnError)(void *ctx, TcpInbox *inbox);

struct TcpInbox {
  int fd;
  Buff buff;
  bool pendingData;
  void *ctx;
  TcpInboxOnData onData;
  TcpInboxOnError onError;
};

/**
 * Inicializa a caixa de entrada tcp.
 *
 * @param  inbox       a caixa de entrada a ser Inicializada.
 * @param  fd          file descritor da conexão.
 * @param  size        capacidade máxima em bytes da caixa.
 * @param  ioevent   monitor de entrada e saida.
 * @param  objListener objeto qualquer para ser repassado para o ouvinte.
 * @param  fnListener  função do ouvinte para tratar os eventos.
 * @return             0, em caso de sucesso, -1, em caso de erros.
 */
int tcpInbox_init(TcpInbox *inbox, int fd, size_t size, void *ctx,
                  TcpInboxOnData onNewData, TcpInboxOnError onError);

BuffReader *tcpInbox_reader(TcpInbox *inbox);

int tcpInbox_close(TcpInbox *inbox);

#endif
