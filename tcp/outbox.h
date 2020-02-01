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

/**
 * Callback para receber notificações de eventos ocorridos na caixa de saída.
 */
typedef void (*TcpOutboxOnError)(void *ctx, TcpOutbox *outbox);
typedef void (*TcpOutboxOnFlushed)(void *ctx, TcpOutbox *outbox);

/**
 * Estrutura da caixa de saída. Em nenhuma hipótese a aplicação deverá acessar
 * os atributos desta estrutura. Toda a operação da caixa de saída deve ser
 * realizada através das funções especificadas neste cabeçalho.
 *
 * @param  fd        [description]
 * @param  size      [description]
 * @param  ctx       [description]
 * @param  onError   [description]
 * @param  onFlushed [description]
 */
struct TcpOutbox {
  int fd;
  Buff buff;
  bool canFlush;
  bool ioeventInstalled;
  void *ctx;
  TcpOutboxOnError onError;
  TcpOutboxOnFlushed onFlushed;
};

/**
 * Inicializa a caixa de saída.
 *
 * @param  outbox    caixa de saída.
 * @param  fd        descritor de socket.
 * @param  size      tamanho em bytes da caixa de saída.
 * @param  ctx       objeto a ser passado para as callbacks.
 * @param  onError   callback chamada em caso de erros na operação da caixa.
 * @param  onFlushed callback chamada quando o conteúdo da caixa for escrito no
 * socket.
 * @return           0, em caso de sucesso, -1, em caso de erro.
 */
int tcpOutbox_init(TcpOutbox *outbox, int fd, size_t size, void *ctx,
                   TcpOutboxOnError onError, TcpOutboxOnFlushed onFlushed);

/**
 * Encerra a conexão com o socket e libera toda a memória utilizada pela caixa
 * de saída.
 *
 * @param  outbox caixa de saída.
 * @return        0, em caso de sucesso, -1, em caso de erro.
 */
int tcpOutbox_close(TcpOutbox *outbox);

/**
 * Escreve no socket todo o conteúdo armazenado na caixa de saída.
 *
 * A aplicação deve chamar esta função apenas quando uma ou mais mensagens
 * estiverem inteiramente armazenadas na caixa de saida.
 *
 * @param outbox caixa de saída.
 */
void tcpOutbox_flush(TcpOutbox *outbox);

/**
 * Obtém um escritor para a caixa de saída. Através dele, a aplicação poderá
 * armazenar dados na caixa de saída. Quando concluir a escrita de uma ou mais
 * mensagens, a aplicação poderá chamar a função tcpOutbox_flush(), a fim de
 * enviar todo o conteúdo da caixa para o socket.
 *
 * @param  outbox caixa de saída
 * @return        escritor de buffer
 */
BuffWriter *tcpOutbox_writer(TcpOutbox *outbox);

#endif
