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
#include "vetor/vetor.h"
#include "log/log.h"
#include "tcp.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

////////////////////////////////////////////////////////////////////////////////

static void fetchData(TcpInbox *inbox);
static void onNewData(void *obj, int fd, IOEventType event);

////////////////////////////////////////////////////////////////////////////////

int tcpInbox_init(TcpInbox *inbox, int fd, size_t size, void *ctx,
                  TcpInboxOnData onData, TcpInboxOnError onError) {
  inbox->fd = fd;
  inbox->ctx = ctx;
  inbox->onData = onData;
  inbox->onError = onError;
  inbox->pendingData = false;

  log_dbug("tcp-inbox", "(fd %d) Criando inbox.\n", inbox->fd);

  if (buff_init(&inbox->buff, size)) {
    log_dbug("tcp-inbox", "(fd %d) Falta de memória na criação da inbox.\n",
             inbox->fd);
    return -1;
  }

  if (ioevent_install(fd, true)) {
    log_dbug("tcp-inbox", "(fd %d) Erro: ioevent_install().\n", inbox->fd);
    return -1;
  }

  if (ioevent_listen(fd, IOEVENT_TYPE_READ, inbox, onNewData)) {
    log_dbug("tcp-inbox", "(fd %d) Erro: ioevent_listen().\n", inbox->fd);
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

BuffReader *tcpInbox_reader(TcpInbox *inbox) {
  return buff_reader(&inbox->buff);
}

////////////////////////////////////////////////////////////////////////////////

int tcpInbox_close(TcpInbox *inbox) {
  log_dbug("tcp-inbox", "(fd %d) Destruindo inbox.\n", inbox->fd);
  int r = 0;

  if (shutdown(inbox->fd, SHUT_RD)) {
    r = -1;
  }

  buff_free(&inbox->buff);

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static void onNewData(void *objListener, int fd, IOEventType event) {
  (void)fd;
  (void)event;

  TcpInbox *inbox = objListener;

  log_dbug("tcp-inbox", "(fd %d) Dados pendentes.\n", inbox->fd);

  inbox->pendingData = true;

  fetchData(inbox);

  return;
}

////////////////////////////////////////////////////////////////////////////////

static void fetchData(TcpInbox *inbox) {
  TcpStatus status;

  log_dbug("tcp-inbox", "(fd %d) Tentando fetchData...\n", inbox->fd);

  if (!inbox->pendingData) {
    log_dbug("tcp-inbox", "(fd %d) Sem dados pendentes.\n", inbox->fd);
    return;
  }

  if (buff_isfull(&inbox->buff)) {
    log_dbug("tcp-inbox", "(fd %d) Caixa cheia.\n", inbox->fd);
    return;
  }

  status = tcp_read(inbox->fd, buff_writer(&inbox->buff));

  if (status != TCP_OK)
    inbox->pendingData = false;

  if (status == TCP_OK || status == TCP_TRY_AGAIN) {
    inbox->onData(inbox->ctx, inbox);
    return;
  }

  if (status == TCP_DISCONNECTED) {
    log_erro("tcp-inbox", "(fd %d) Cliente fechou a conexão.\n", inbox->fd);
    inbox->onError(inbox->ctx, inbox);
    return;
  }

  if (status == TCP_ERROR) {
    log_erro("tcp-inbox", "(fd %d) Erro na leitura da conexão.\n", inbox->fd);
    inbox->onError(inbox->ctx, inbox);
    return;
  }
}
