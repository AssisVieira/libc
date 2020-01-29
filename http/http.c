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

#include "http.h"
#include "buff/buff.h"
#include "ioevent/ioevent.h"
#include "log/log.h"
#include "tcp/inbox.h"
#include "tcp/outbox.h"
#include "tcp/port.h"
#include "tcp/tcp.h"
#include <assert.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

typedef enum HttpReqState {
  REQ_OK = 0,
  REQ_ERROR,
  REQ_INCOMPLETE,
} HttpReqState;

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpHeader {
  char name[HEADER_NAME_MAX];
  char value[HEADER_VALUE_MAX];
  size_t nameLen;
  size_t valueLen;
} HttpHeader;

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpParam {
  char name[PARAM_NAME_MAX];
  char value[PARAM_VALUE_MAX];
  size_t nameLen;
  size_t valueLen;
} HttpParam;

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpReq {
  char uri[URI_MAX];
  size_t uriLen;

  char method[METHOD_MAX];
  size_t methodLen;

  int versionMajor;
  int versionMinor;

  HttpHeader headers[HEADERS_MAX];
  size_t headersLen;

  HttpParam params[PARAMS_MAX];
  size_t paramsLen;

  char args[ARGS_MAX][ARG_MAX];
  size_t argsLen;
} HttpReq;

////////////////////////////////////////////////////////////////////////////////

typedef int (*HttpClientStateFn)(HttpClient *client, char c);

////////////////////////////////////////////////////////////////////////////////

struct HttpClient {
  HttpClientStateFn state;
  HttpReq req;
  char args[ARGS_MAX][ARG_MAX];
  bool respHeaderDone;
  int fd;
  TcpInbox inbox;
  TcpOutbox outbox;
  bool acceptRequest;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpHandler {
  regex_t pattern;
  char method[METHOD_MAX];
  HttpHandlerFunc func;
} HttpHandler;

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpServer {
  TcpPort port;
  bool closing;
  Vetor *clients; // Vector of HttpClient *
  HttpHandler handlers[HANDLERS_MAX];
  size_t handlersLen;
} HttpServer;

////////////////////////////////////////////////////////////////////////////////

static thread_local HttpServer server = {
    .closing = false,
    .clients = NULL,
    .handlersLen = 0,
};

////////////////////////////////////////////////////////////////////////////////

static void http_onConnected(TcpPort *port, int conexao);
static void http_onRequest(HttpClient *client);
static int http_close();
static int http_dispatch(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static int stateMetodoInicio(HttpClient *client, char c);
static int stateMetodo(HttpClient *client, char c);
static int stateUri(HttpClient *client, char c);
static int stateParamNome(HttpClient *client, char c);
static int stateParamValor(HttpClient *client, char c);
static int stateParamNovo(HttpClient *client, char c);
static int stateHttpVersionH(HttpClient *client, char c);
static int stateHttpVersionT1(HttpClient *client, char c);
static int stateHttpVersionT2(HttpClient *client, char c);
static int stateHttpVersionP(HttpClient *client, char c);
static int stateHttpVersionSlash(HttpClient *client, char c);
static int stateHttpVersao1Inicio(HttpClient *client, char c);
static int stateHttpVersao1(HttpClient *client, char c);
static int stateHttpVersao2Inicio(HttpClient *client, char c);
static int stateHttpVersao2(HttpClient *client, char c);
static int stateNovaLinha1(HttpClient *client, char c);
static int stateCabecalhoNovo(HttpClient *client, char c);
static int stateCabecalhoEspacoAntesDoValor(HttpClient *client, char c);
static int stateCabecalhoNome(HttpClient *client, char c);
static int stateCabecalhoValor(HttpClient *client, char c);
static int stateNovaLinha2(HttpClient *client, char c);
static int stateNovaLinha3(HttpClient *client, char c);

////////////////////////////////////////////////////////////////////////////////

static bool http_isCtl(int c);
static bool http_isDigit(int c);
static bool http_isLetter(char c);

////////////////////////////////////////////////////////////////////////////////

static HttpReqState http_formatReq(HttpClient *client, BuffReader *reader);
static void http_acceptRequest(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static void http_onOutboxError(void *ctx, TcpOutbox *outbox);
static void http_onOutboxFlushed(void *ctx, TcpOutbox *outbox);
static void http_onInboxData(void *ctx, TcpInbox *inbox);
static void http_onInboxError(void *ctx, TcpInbox *inbox);
static void http_sendRequest(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static const char *http_strMimeType(HttpMimeType contentType);
static const char *http_strStatus(HttpStatus status);
static int http_respHeaderEnd(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static int http_closeClient(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static size_t http_min(size_t a, size_t b);

////////////////////////////////////////////////////////////////////////////////

int http_open(const char *port, size_t maxClients) {
  int r = 0;
  bool finish = false;

  log_info("http", "Inicializando...\n");
  log_info("http", "Concorrência máxima: %d\n", maxClients);

  server.closing = false;
  server.clients = vetor_criar(maxClients);

  if (server.clients == NULL) {
    return -1;
  }

  if (ioevent_open()) {
    return -1;
  }

  if (tcpPort_open(&server.port, port, maxClients, http_onConnected)) {
    return -1;
  }

  log_info("http", "Aguardando conexões na porta: %s.\n", port);

  if (ioevent_run(&finish)) {
    log_erro("http", "Erro em ioevent_run().\n");
    r = -1;
  }

  if (http_close(server)) {
    log_erro("http", "Erro em closeServer().\n");
    r = -1;
  }

  return r;
}

////////////////////////////////////////////////////////////////////////////////

int http_handler(const char *method, const char *path, HttpHandlerFunc func) {
  HttpHandler *handler = &server.handlers[server.handlersLen++];

  if (regcomp(&handler->pattern, path, REG_EXTENDED)) {
    log_erro("http", "Erro ao compilar expressão regular: %s.\n", path);
    server.handlersLen--;
    return -1;
  }

  handler->func = func;

  handler->method[0] = '\0';
  strncat(handler->method, method, METHOD_MAX - 1);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int http_close() {
  int r = 0;

  log_dbug("http", "Encerrando servidor...\n");

  server.closing = true;

  if (tcpPort_close(&server.port)) {
    r = -1;
  }

  for (size_t i = 0; i < vetor_qtd(server.clients); i++) {
    HttpClient *client = vetor_item(server.clients, i);
    if (client != NULL) {
      if (http_closeClient(client)) {
        r = -1;
      }
      vetor_inserir(server.clients, i, NULL);
    }
  }

  vetor_destruir(server.clients);
  server.clients = NULL;

  for (int i = 0; i < server.handlersLen; i++) {
    regfree(&server.handlers[i].pattern);
  }

  log_dbug("http", "Tchau!\n");

  return r;
}

/////////////////////////////////////////////////////////////////////////////////

static void http_onConnected(TcpPort *port, int fd) {
  if (server.closing) {
    log_dbug("http", "Ignorando cliente (servidor esta encerrando): %d.\n", fd);
    return;
  }

  if (vetor_item(server.clients, fd) != NULL) {
    log_erro("http",
             "Novo cliente tentando substituir um cliente existente: %d\n", fd);
    return;
  }

  HttpClient *client = malloc(sizeof(HttpClient));

  if (client == NULL) {
    log_erro("http", "Erro: malloc()\n");
    return;
  }

  http_acceptRequest(client);

  client->fd = fd;

  if (tcpInbox_init(&client->inbox, fd, TCP_INBOX_SIZE, client,
                    http_onInboxData, http_onInboxError)) {
    log_erro("http", "Erro em tcpInbox_init() - fd: %d.\n", fd);
    goto error;
  }

  if (tcpOutbox_init(&client->outbox, fd, TCP_OUTBOX_MAX_SIZE, client,
                     http_onOutboxError, http_onOutboxFlushed)) {
    log_erro("http", "Errohttp_ em tcpOutbox_init() - fd: %d.\n", fd);
    goto error;
  }

  log_dbug("http", "Novo cliente: %d\n", client->fd);

  vetor_inserir(server.clients, fd, client);

  return;

error:
  if (close(client->fd)) {
    log_erro("http", "Erro ao fechar a conexão: %d\n", client->fd);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void http_onRequest(HttpClient *client) {
  if (server.closing) {
    log_dbug("http", "Ignorando requisição: %s.\n", http_reqPath(client));
    return;
  }

  log_info("http", "%s %s\n", http_reqMethod(client), http_reqPath(client));

  if (http_dispatch(client)) {
    log_dbug("http", "Recurso não encontrado: %s.\n", http_reqPath(client));
    http_respNotFound(client, HTTP_TYPE_HTML, "Recurso não encontrado");
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

static size_t http_min(size_t a, size_t b) { return (a > b) ? b : a; }

////////////////////////////////////////////////////////////////////////////////

static int http_dispatch(HttpClient *client) {
  const HttpHandler *matchedHandler = NULL;
  regmatch_t args[ARGS_MAX];

  for (int i = 0; i < server.handlersLen; i++) {
    HttpHandler *handler = &server.handlers[i];

    if (strcmp(handler->method, http_reqMethod(client)) == 0 &&
        regexec(&handler->pattern, http_reqPath(client), ARGS_MAX, args, 0) ==
            0) {

      for (size_t i = 1; i < ARGS_MAX && args[i].rm_so != -1; i++) {
        client->req.argsLen++;
        client->req.args[i - 1][0] = '\0';

        strncat(client->req.args[i - 1], http_reqPath(client) + args[i].rm_so,
                http_min(ARG_MAX - 1, args[i].rm_eo - args[i].rm_so));

        log_dbug("http", "Argumento: %s\n", client->req.args[i - 1]);
      }

      matchedHandler = handler;

      break;
    }
  }

  if (matchedHandler == NULL) {
    return -1;
  }

  matchedHandler->func(client);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void http_onOutboxFlushed(void *ctx, TcpOutbox *outbox) {
  HttpClient *client = ctx;
  log_dbug("http", "Caixa de saída esvaziada (fd: %d)\n", client->fd);
  http_acceptRequest(client);
}

static void http_onOutboxError(void *ctx, TcpOutbox *outbox) {
  (void)outbox;
  HttpClient *client = ctx;
  http_closeClient(client);
}

////////////////////////////////////////////////////////////////////////////////

static void http_onInboxData(void *ctx, TcpInbox *inbox) {
  HttpClient *client = ctx;
  BuffReader *reader = tcpInbox_reader(inbox);

  log_dbug("http", "onInboxData() - fd: %d\n", client->fd);

  if (!client->acceptRequest) {
    log_dbug("http", "onInboxData() - fd: %d\n", client->fd);
    return;
  }

  HttpReqState state = http_formatReq(client, reader);

  if (state == REQ_OK) {
    http_onRequest(client);
    return;
  }

  if (state == REQ_ERROR) {
    http_closeClient(client);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void http_onInboxError(void *ctx, TcpInbox *inbox) {
  HttpClient *client = ctx;

  log_dbug("http", "onInboxError() - fd: %d\n", client->fd);

  http_closeClient(client);
}

////////////////////////////////////////////////////////////////////////////////

static int http_closeClient(HttpClient *client) {
  int r = 0;

  if (tcpInbox_close(&client->inbox)) {
    log_erro("http", "Erro ao fechar a inbox: %d\n", client->fd);
    r = -1;
  }

  if (tcpOutbox_close(&client->outbox)) {
    log_erro("http", "Erro ao fechar a outbox: %d\n", client->fd);
    r = -1;
  }

  if (close(client->fd)) {
    log_erro("http", "Erro ao destruir a conexão: %d\n", client->fd);
    r = -1;
  }

  vetor_inserir(server.clients, (size_t)client->fd, NULL);

  tcpPort_onClientDisconnected(&server.port);

  log_dbug("http", "Conexão destruida: %d\n", client->fd);

  free(client);

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static void http_acceptRequest(HttpClient *client) {
  client->req.uriLen = 0;
  client->req.methodLen = 0;
  client->req.versionMinor = 0;
  client->req.versionMajor = 0;

  client->req.headersLen = 0;
  memset(client->req.headers, 0x0, sizeof(client->req.headers));

  client->req.paramsLen = 0;
  memset(client->req.params, 0x0, sizeof(client->req.params));

  client->req.argsLen = 0;

  client->state = stateMetodoInicio;

  client->respHeaderDone = false;

  client->acceptRequest = true;
}

////////////////////////////////////////////////////////////////////////////////

static HttpReqState http_formatReq(HttpClient *client, BuffReader *reader) {
  const char *c;
  size_t r;

  log_dbug("http", "Tentanto formatar.\n");

  while ((r = buff_reader_read(reader, &c, 1)) > 0) {
    HttpReqState state = client->state(client, *c);

    if (state != REQ_INCOMPLETE) {
      return state;
    }
  }

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static bool http_isCtl(int c) { return (c >= 0 && c <= 31) || (c == 127); }

////////////////////////////////////////////////////////////////////////////////

static bool http_isLetter(char c) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    return true;
  else
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool http_isDigit(int c) { return c >= '0' && c <= '9'; }

////////////////////////////////////////////////////////////////////////////////

static int stateMetodoInicio(HttpClient *client, char c) {
  if (c < 'A' || c > 'Z') {
    log_erro("http",
             "stateMetodoInicio() - esperando a letra (A-Z), mas veio: %c.\n",
             c);
    return REQ_ERROR;
  }

  client->state = stateMetodo;

  client->req.method[client->req.methodLen++] = c;
  client->req.method[client->req.methodLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateMetodo(HttpClient *client, char c) {
  if (c == ' ') {
    log_dbug("http", "Método: %s\n", client->req.method);
    client->state = stateUri;
    return REQ_INCOMPLETE;
  }

  if (c < 'A' || c > 'Z') {
    log_erro("http", "stateMetodo() - esperando a letra (A-Z), mas veio: %c.\n",
             c);
    return REQ_ERROR;
  }

  client->req.method[client->req.methodLen++] = c;
  client->req.method[client->req.methodLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateUri(HttpClient *client, char c) {
  if (c == ' ') {
    log_dbug("http", "Uri: %s\n", client->req.uri);
    client->state = stateHttpVersionH;
    return REQ_INCOMPLETE;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateUri() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return REQ_ERROR;
  }

  if (c == '?') {
    client->state = stateParamNovo;
    return REQ_INCOMPLETE;
  }

  client->req.uri[client->req.uriLen++] = c;
  client->req.uri[client->req.uriLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateParamNome(HttpClient *client, char c) {
  HttpParam *param = &client->req.params[client->req.paramsLen - 1];

  if (c == ' ') {
    log_dbug("http", "Nome de parâmetro: %s\n", param->name);
    client->state = stateHttpVersionH;
    return REQ_INCOMPLETE;
  }

  if (c == '=') {
    log_dbug("http", "Nome de parâmetro: %s\n", param->name);
    client->state = stateParamValor;
    return REQ_INCOMPLETE;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamNome() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return REQ_ERROR;
  }

  if (param->nameLen >= PARAM_NAME_MAX) {
    log_erro("http",
             "stateParamNome() - nome do parâmetro maior do que o permitido: "
             "%ld\n",
             PARAM_NAME_MAX);
    return REQ_ERROR;
  }

  param->name[param->nameLen++] = c;
  param->name[param->nameLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateParamValor(HttpClient *client, char c) {
  HttpParam *param = &client->req.params[client->req.paramsLen - 1];

  if (c == ' ') {
    log_dbug("http", "Valor de parâmetro: %s\n", param->value);
    client->state = stateHttpVersionH;
    return REQ_INCOMPLETE;
  }

  if (c == '&') {
    log_dbug("http", "Valor de parâmetro: %s\n", param->value);
    client->state = stateParamNovo;
    return REQ_INCOMPLETE;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamValor() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return REQ_ERROR;
  }

  if (param->valueLen >= PARAM_VALUE_MAX) {
    log_erro("http",
             "stateParamValor() - valor do parâmetro maior do que o "
             "permitido: %ld.\n",
             PARAM_VALUE_MAX);
    return REQ_ERROR;
  }

  param->value[param->valueLen++] = c;
  param->value[param->valueLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateParamNovo(HttpClient *client, char c) {
  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamNovo() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return REQ_ERROR;
  }

  if (client->req.paramsLen >= PARAMS_MAX) {
    log_erro("http",
             "stateParamNovo() - quantidade de parâmetros maior do que o "
             "permitido: %ld\n",
             PARAMS_MAX);
    return REQ_ERROR;
  }

  log_dbug("http", "stateParamNovo()\n");

  HttpParam *param = &client->req.params[client->req.paramsLen++];
  param->name[param->nameLen++] = c;
  param->name[param->nameLen] = '\0';

  client->state = stateParamNome;
  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersionH(HttpClient *client, char c) {
  if (c == 'H') {
    client->state = stateHttpVersionT1;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersionH() - esperando a letra 'H', mas veio: %c.\n", c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersionT1(HttpClient *client, char c) {
  if (c == 'T') {
    client->state = stateHttpVersionT2;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersionT1() - esperando a letra 'T', mas veio: %c.\n", c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersionT2(HttpClient *client, char c) {
  if (c == 'T') {
    client->state = stateHttpVersionP;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersionT2() - esperando a letra 'T', mas veio: %c.\n", c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersionP(HttpClient *client, char c) {
  if (c == 'P') {
    client->state = stateHttpVersionSlash;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersionP() - esperando a letra 'P', mas veio: %c.\n", c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersionSlash(HttpClient *client, char c) {
  if (c == '/') {
    client->req.versionMajor = 0;
    client->req.versionMinor = 0;
    client->state = stateHttpVersao1Inicio;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersionSlash() - esperando barra invertida (/), mas "
           "veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersao1Inicio(HttpClient *client, char c) {
  if (http_isDigit(c)) {
    client->req.versionMajor *= 10 + (unsigned)c - '0';
    client->state = stateHttpVersao1;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersao1Inicio() - esperando dígito (0-9), mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersao1(HttpClient *client, char c) {
  if (c == '.') {
    client->state = stateHttpVersao2Inicio;
    return REQ_INCOMPLETE;
  } else if (http_isDigit(c)) {
    client->req.versionMajor *= 10 + (unsigned)c - '0';
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersao1() - esperando sinal de ponto-final (.) ou "
           "dígito (0-9), mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersao2Inicio(HttpClient *client, char c) {
  if (http_isDigit(c)) {
    client->req.versionMinor *= 10 + (unsigned)c - '0';
    client->state = stateHttpVersao2;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersao2Inicio() - esperando dígito (0-9), mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateHttpVersao2(HttpClient *client, char c) {
  if (c == '\r') {
    client->state = stateNovaLinha1;
    return REQ_INCOMPLETE;
  } else if (http_isDigit(c)) {
    client->req.versionMinor *= 10 + (unsigned)c - '0';
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateHttpVersao2() - esperando retorno de cursor '\\r' ou dígito "
           "(0-9), mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateNovaLinha1(HttpClient *client, char c) {
  if (c == '\n') {
    client->state = stateCabecalhoNovo;
    return REQ_INCOMPLETE;
  }

  log_erro("http", "stateNovaLinha1() - esperando nova linha, mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateCabecalhoNovo(HttpClient *client, char c) {
  if (c == '\r') {
    client->state = stateNovaLinha3;
    return REQ_INCOMPLETE;
  }

  if (!http_isLetter(c)) {
    log_erro("http", "stateCabecalhoNovo() - esperando letra, mas veio: %c.\n",
             c);
    return REQ_ERROR;
  }

  if (client->req.headersLen >= HEADERS_MAX) {
    log_erro("http",
             "stateCabecalhoNovo() - quantidade de cabecalhos maior do que o "
             "permitido: %ld.\n",
             HEADERS_MAX);
    return REQ_ERROR;
  }

  HttpHeader *header = &client->req.headers[client->req.headersLen++];
  header->name[header->nameLen++] = c;
  header->name[header->nameLen] = '\0';

  client->state = stateCabecalhoNome;
  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateCabecalhoEspacoAntesDoValor(HttpClient *client, char c) {
  if (c == ' ') {
    client->state = stateCabecalhoValor;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateCabecalhoEspacoAntesDoValor() - esperando espaço em branco, "
           "mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateCabecalhoNome(HttpClient *client, char c) {
  HttpHeader *header = &client->req.headers[client->req.headersLen - 1];

  if (c == ':') {
    log_dbug("http", "Nome de cabeçalho: %s\n", header->name);
    client->state = stateCabecalhoEspacoAntesDoValor;
    return REQ_INCOMPLETE;
  }

  if (!http_isLetter(c) && c != '-') {
    log_erro("http",
             "stateCabecalhoNome() - esperando letra ou traço, mas "
             "veio: %c.\n",
             c);
    return REQ_ERROR;
  }

  if (header->nameLen >= HEADER_NAME_MAX) {
    log_erro("http",
             "stateCabecalhoNome() - nome do cabeçalho maior do que o "
             "permitido: %ld.\n",
             HEADER_NAME_MAX);
    return REQ_ERROR;
  }

  header->name[header->nameLen++] = c;
  header->name[header->nameLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateCabecalhoValor(HttpClient *client, char c) {
  HttpHeader *header = &client->req.headers[client->req.headersLen - 1];

  if (c == '\r') {
    log_dbug("http", "Valor de cabeçalho: %s\n", header->value);
    client->state = stateNovaLinha2;
    return REQ_INCOMPLETE;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateCabecalhoValor() - esperando letra, número ou símbolo, "
             "mas veio caractere de controle: %02X.\n",
             c);
    return REQ_ERROR;
  }

  if (header->valueLen >= HEADER_VALUE_MAX) {
    log_erro("http",
             "stateCabecalhoValor() - valor do cabeçalho maior do que o "
             "permitido: %ld.\n",
             HEADER_VALUE_MAX);
    return REQ_ERROR;
  }

  header->value[header->valueLen++] = c;
  header->value[header->valueLen] = '\0';

  return REQ_INCOMPLETE;
}

////////////////////////////////////////////////////////////////////////////////

static int stateNovaLinha2(HttpClient *client, char c) {
  if (c == '\n') {
    client->state = stateCabecalhoNovo;
    return REQ_INCOMPLETE;
  }

  log_erro("http",
           "stateNovaLinha2() - esperando nova linha '\\n', mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static int stateNovaLinha3(HttpClient *client, char c) {
  (void)client;

  if (c == '\n') {
    return REQ_OK;
  }

  log_erro("http",
           "stateNovaLinha3() - esperando nova linha '\\n', mas veio: %c.\n",
           c);

  return REQ_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

int http_respNotFound(HttpClient *client, HttpMimeType mimeType,
                      const char *str) {
  http_respBegin(client, HTTP_STATUS_NOT_FOUND, mimeType);
  http_respBody(client, str);
  http_respEnd(client);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respOk(HttpClient *client, HttpMimeType mimeType, const char *str) {
  http_respBegin(client, HTTP_STATUS_OK, mimeType);
  http_respBody(client, str);
  http_respEnd(client);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respBegin(HttpClient *client, HttpStatus status,
                   HttpMimeType mimeType) {
  log_dbug("http", "<<< HTTP/1.1 %d %s\\r\\n\n", status,
           http_strStatus(status));
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);

  switch (status) {
  case HTTP_STATUS_OK:
    buff_writer_write(writer, "HTTP/1.1 200 OK\r\n",
                      strlen("HTTP/1.1 200 OK\r\n"));
    break;
  case HTTP_STATUS_NOT_FOUND:
    buff_writer_write(writer, "HTTP/1.1 404 OK\r\n",
                      strlen("HTTP/1.1 404 OK\r\n"));
    break;
  case HTTP_STATUS_BAD_REQUEST:
    buff_writer_write(writer, "HTTP/1.1 400 OK\r\n",
                      strlen("HTTP/1.1 400 OK\r\n"));
    break;
  }

  http_respHeader(client, "Content-Type", http_strMimeType(mimeType));
  http_respHeader(client, "Transfer-Encoding", "chunked");

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respHeader(HttpClient *client, const char *nome, const char *valor) {
  log_dbug("http", "<<< %s: %s\\r\\n\n", nome, valor);
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  buff_writer_write(writer, nome, strlen(nome));
  buff_writer_write(writer, ": ", 2);
  buff_writer_write(writer, valor, strlen(valor));
  buff_writer_write(writer, "\r\n", 2);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respHeaderInt(HttpClient *client, const char *nome, int valor) {
  log_dbug("http", "<<< %s: %s\\r\\n\n", nome, valor);
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  buff_writer_write(writer, nome, strlen(nome));
  buff_writer_write(writer, ": ", 2);
  buff_writer_printf(writer, "%d", valor);
  buff_writer_write(writer, "\r\n", 2);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int http_respHeaderEnd(HttpClient *client) {
  log_dbug("http", "<<< \\r\\n\n");
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  buff_writer_write(writer, "\r\n", 2);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respBody(HttpClient *client, const char *fmt, ...) {
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  va_list vargs;
  va_start(vargs, fmt);

  char buff[2000];
  int buffLen = vsnprintf(buff, sizeof(buff), fmt, vargs);

  if (buffLen > sizeof(buff)) {
    return -1;
  }

  if (!client->respHeaderDone) {
    client->respHeaderDone = true;
    http_respHeaderEnd(client);
  }

  buff_writer_printf(writer, "%X\r\n", buffLen);
  buff_writer_write(writer, buff, buffLen);
  buff_writer_write(writer, "\r\n", 2);

  va_end(vargs);

  log_dbug("http", "<<< %d\\r\\n\n", buffLen);
  log_dbug("http", "<<< %s\\r\\n\n", buff);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respBodyLen(HttpClient *client, const char *buff, size_t len) {
  log_dbug("http", "<<< %d\\r\\n\n", len);
  log_dbugbin("http", buff, len, "<<< ");
  log_dbug("http", "<<< \\r\\n\n", len);

  if (!client->respHeaderDone) {
    client->respHeaderDone = true;
    http_respHeaderEnd(client);
  }

  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  buff_writer_printf(writer, "%X\r\n", len);
  buff_writer_write(writer, buff, len);
  buff_writer_write(writer, "\r\n", 2);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int http_respEnd(HttpClient *client) {
  log_dbug("http", "<<< 0\\r\\n\n");
  log_dbug("http", "<<< \\r\\n\n");
  BuffWriter *writer = tcpOutbox_writer(&client->outbox);
  buff_writer_write(writer, "0\r\n\r\n", 5);
  http_sendRequest(client);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void http_sendRequest(HttpClient *client) {
  tcpOutbox_flush(&client->outbox);
}

////////////////////////////////////////////////////////////////////////////////

static const char *http_strStatus(HttpStatus status) {
  switch (status) {
  case HTTP_STATUS_OK:
    return "Ok";
  case HTTP_STATUS_NOT_FOUND:
    return "Not Found";
  case HTTP_STATUS_BAD_REQUEST:
    return "Bad Request";
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static const char *http_strMimeType(HttpMimeType contentType) {
  switch (contentType) {
  case HTTP_TYPE_HTML:
    return "text/html; charset=utf8";
  case HTTP_TYPE_JSON:
    return "text/json; charset=utf8";
  case HTTP_TYPE_TEXT:
    return "text/plain; charset=utf8";
  case HTTP_TYPE_CSS:
    return "text/css; charset=utf8";
  case HTTP_TYPE_JS:
    return "text/javascript; charset=utf8";
  case HTTP_TYPE_JPEG:
    return "image/jpeg; charset=utf8";
  case HTTP_TYPE_PNG:
    return "image/png; charset=utf8";
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqHeader(const HttpClient *client, const char *name) {
  for (int i = 0; i < client->req.headersLen; i++) {
    if (strcmp(client->req.headers[i].name, name) == 0) {
      return client->req.headers[i].value;
    }
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqParam(const HttpClient *client, const char *name) {
  for (int i = 0; i < client->req.paramsLen; i++) {
    if (strcmp(client->req.params[i].name, name) == 0) {
      return client->req.params[i].value;
    }
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqMethod(const HttpClient *client) {
  return client->req.method;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqPath(const HttpClient *client) { return client->req.uri; }

////////////////////////////////////////////////////////////////////////////////

const char *http_reqArg(const HttpClient *client, int n) {
  if (n >= client->req.argsLen) {
    return "";
  }
  return client->req.args[n];
}
