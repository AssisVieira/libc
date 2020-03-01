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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "buff/buff.h"
#include "log/log.h"
#include "server/server.h"
#include "str/str.h"
#include "vetor/vetor.h"

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

typedef FormatStatus (*HttpReqStateFn)(HttpClient *client, char c);

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpReq {
  HttpReqStateFn state;

  char uri[URI_MAX];
  int uriLen;

  char method[METHOD_MAX];
  int methodLen;

  int versionMajor;
  int versionMinor;

  HttpHeader headers[HEADERS_MAX];
  int headersLen;

  HttpParam params[PARAMS_MAX];
  int paramsLen;

  char args[ARGS_MAX][ARG_MAX];
  int argsLen;

  char body[BODY_MAX];
  int bodyLen;

  int contentLength;

  // Transient, pattern used to route the request.
  const char *pattern;
} HttpReq;

////////////////////////////////////////////////////////////////////////////////

struct HttpClient {
  HttpReq *req;
  str_t *resp;
  int fd;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpHandler {
  regex_t pattern;
  char patternRaw[PATTERN_MAX];
  char method[METHOD_MAX];
  HttpHandlerFunc func;
} HttpHandler;

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpServer {
  Vetor *clients;  // Vector of HttpClient *
  HttpHandler handlers[HANDLERS_MAX];
  size_t handlersLen;
} HttpServer;

////////////////////////////////////////////////////////////////////////////////

static thread_local HttpServer *http = NULL;

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateMetodoInicio(HttpClient *client, char c);
static FormatStatus stateMetodo(HttpClient *client, char c);
static FormatStatus stateUri(HttpClient *client, char c);
static FormatStatus stateParamNome(HttpClient *client, char c);
static FormatStatus stateParamValor(HttpClient *client, char c);
static FormatStatus stateParamNovo(HttpClient *client, char c);
static FormatStatus stateHttpVersionH(HttpClient *client, char c);
static FormatStatus stateHttpVersionT1(HttpClient *client, char c);
static FormatStatus stateHttpVersionT2(HttpClient *client, char c);
static FormatStatus stateHttpVersionP(HttpClient *client, char c);
static FormatStatus stateHttpVersionSlash(HttpClient *client, char c);
static FormatStatus stateHttpVersao1Inicio(HttpClient *client, char c);
static FormatStatus stateHttpVersao1(HttpClient *client, char c);
static FormatStatus stateHttpVersao2Inicio(HttpClient *client, char c);
static FormatStatus stateHttpVersao2(HttpClient *client, char c);
static FormatStatus stateNovaLinha1(HttpClient *client, char c);
static FormatStatus stateCabecalhoNovo(HttpClient *client, char c);
static FormatStatus stateCabecalhoEspacoAntesDoValor(HttpClient *client,
                                                     char c);
static FormatStatus stateCabecalhoNome(HttpClient *client, char c);
static FormatStatus stateCabecalhoValor(HttpClient *client, char c);
static FormatStatus stateNovaLinha2(HttpClient *client, char c);
static FormatStatus stateNovaLinha3(HttpClient *client, char c);
static FormatStatus stateBody(HttpClient *client, char c);

////////////////////////////////////////////////////////////////////////////////

static bool http_isCtl(int c);
static bool http_isDigit(int c);
static bool http_isLetter(char c);

////////////////////////////////////////////////////////////////////////////////

static FormatStatus http_onFormat(int clientFd, BuffReader *reader);
static void http_onMessage(int clientFd);
static void http_onDisconnected(int clientFd);
static void http_onConnected(int clientFd);
static void http_onClean(int clientFd);
static int http_dispatch(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////

static const char *http_strMimeType(HttpMimeType contentType);
static const char *http_strStatus(HttpStatus status);
static size_t http_min(size_t a, size_t b);

////////////////////////////////////////////////////////////////////////////////

static HttpClient *http_newClient(int clientFd);
static void http_freeClient(HttpClient *client);
static void http_clearClient(HttpClient *client);
static HttpClient *http_client(int clientFd);

////////////////////////////////////////////////////////////////////////////////

static HttpReq *http_newReq();
static void http_freeReq(HttpReq *req);
static void http_clearReq(HttpReq *req);
static int http_init();
static void http_free();

////////////////////////////////////////////////////////////////////////////////

int http_start(int port, int maxClients) {
  int r = 0;

  // Startup...

  if (http_init()) {
    return -1;
  }

  ServerParams params;
  params.host = "127.0.0.1";
  params.port = port;
  params.inboxMaxSize = INBOX_MAX_SIZE;
  params.outboxInitSize = OUTBOX_INIT_SIZE;
  params.maxClients = maxClients;
  params.onConnected = http_onConnected;
  params.onFormat = http_onFormat;
  params.onMessage = http_onMessage;
  params.onDisconnected = http_onDisconnected;
  params.onClean = http_onClean;

  // Execution...

  r = server_start(params);

  // Shutdown...

  http_free();

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static int http_init() {
  if (http != NULL) return 0;

  http = malloc(sizeof(HttpServer));

  if (http == NULL) {
    return -1;
  }

  http->clients = vetor_criar(HTTP_CLIENTS_INIT_SIZE);

  if (http->clients == NULL) {
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void http_free() {
  for (size_t i = 0; i < vetor_qtd(http->clients); i++) {
    HttpClient *client = vetor_item(http->clients, i);
    http_freeClient(client);
  }

  vetor_destruir(http->clients);
  http->clients = NULL;

  for (int i = 0; i < http->handlersLen; i++) {
    regfree(&http->handlers[i].pattern);
  }

  free(http);

  http = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static void http_freeClient(HttpClient *client) {
  if (client == NULL) return;
  http_freeReq(client->req);
  str_free(&client->resp);
  free(client);
}

////////////////////////////////////////////////////////////////////////////////

int http_handler(const char *method, const char *pattern,
                 HttpHandlerFunc func) {
  if (http_init()) {
    return -1;
  }

  HttpHandler *handler = &http->handlers[http->handlersLen++];

  if (regcomp(&handler->pattern, pattern, REG_EXTENDED)) {
    log_erro("http", "Erro ao compilar expressão regular: %s.\n", pattern);
    http->handlersLen--;
    return -1;
  }

  strncpy(handler->patternRaw, pattern, PATTERN_MAX);

  handler->func = func;

  handler->method[0] = '\0';
  strncat(handler->method, method, METHOD_MAX - 1);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void http_stop(int result) {
  log_dbug("http", "Closing...\n");
  server_stop(result);
}

////////////////////////////////////////////////////////////////////////////////

static void http_onConnected(int clientFd) {
  HttpClient *client = http_client(clientFd);

  if (client == NULL) {
    log_erro("http", "http_client()");
    server_close(clientFd);
    return;
  }

  log_dbug("http", "Client connected: %d\n", clientFd);
}

////////////////////////////////////////////////////////////////////////////////

static void http_onClean(int clientFd) {
  HttpClient *client = http_client(clientFd);

  http_clearClient(client);

  log_dbug("http", "Client connected: %d\n", clientFd);
}

////////////////////////////////////////////////////////////////////////////////

static void http_onDisconnected(int clientFd) {
  log_dbug("http", "Client disconnected: %d\n", clientFd);
}

////////////////////////////////////////////////////////////////////////////////

static void http_clearClient(HttpClient *client) {
  http_clearReq(client->req);
  str_clear(client->resp);
}

////////////////////////////////////////////////////////////////////////////////

static HttpClient *http_client(int clientFd) {
  HttpClient *client = vetor_item(http->clients, clientFd);
  if (client == NULL) {
    client = http_newClient(clientFd);
    vetor_inserir(http->clients, clientFd, client);
  }
  return client;
}

////////////////////////////////////////////////////////////////////////////////

static HttpClient *http_newClient(int clientFd) {
  HttpClient *client = malloc(sizeof(HttpClient));

  if (client == NULL) {
    log_erro("http", "malloc() - %d - %s\n", errno, strerror(errno));
    return NULL;
  }

  client->fd = clientFd;
  client->req = http_newReq();
  client->resp = str_new(HTTP_RESP_INIT_SIZE);
  return client;
}

////////////////////////////////////////////////////////////////////////////////

static void http_onMessage(int clientFd) {
  HttpClient *client = http_client(clientFd);

  log_info("http", "%s %s\n", http_reqMethod(clientFd), http_reqPath(clientFd));

  if (http_dispatch(client)) {
    log_dbug("http", "Recurso não encontrado: %s.\n", http_reqPath(clientFd));
    http_sendNotFound(clientFd);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

static size_t http_min(size_t a, size_t b) { return (a > b) ? b : a; }

////////////////////////////////////////////////////////////////////////////////

static int http_dispatch(HttpClient *client) {
  const HttpHandler *matchedHandler = NULL;
  regmatch_t args[ARGS_MAX];

  for (int i = 0; i < http->handlersLen; i++) {
    HttpHandler *handler = &http->handlers[i];

    if (strcmp(handler->method, http_reqMethod(client->fd)) == 0 &&
        regexec(&handler->pattern, http_reqPath(client->fd), ARGS_MAX, args,
                0) == 0) {
      client->req->pattern = handler->patternRaw;

      for (size_t i = 1; i < ARGS_MAX && args[i].rm_so != -1; i++) {
        client->req->argsLen++;
        client->req->args[i - 1][0] = '\0';

        strncat(client->req->args[i - 1],
                http_reqPath(client->fd) + args[i].rm_so,
                http_min(ARG_MAX - 1, args[i].rm_eo - args[i].rm_so));

        log_dbug("http", "Argumento: %s\n", client->req->args[i - 1]);
      }

      matchedHandler = handler;

      break;
    }
  }

  if (matchedHandler == NULL) {
    return -1;
  }

  matchedHandler->func(client->fd);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static HttpReq *http_newReq() {
  HttpReq *req = malloc(sizeof(HttpReq));

  if (req == NULL) {
    log_erro("http", "malloc(): %d -  %s\n", errno, strerror(errno));
    return NULL;
  }

  http_clearReq(req);

  return req;
}

////////////////////////////////////////////////////////////////////////////////

static void http_freeReq(HttpReq *req) { free(req); }

////////////////////////////////////////////////////////////////////////////////

static void http_clearReq(HttpReq *req) {
  req->uriLen = 0;
  req->methodLen = 0;
  req->versionMinor = 0;
  req->versionMajor = 0;

  req->headersLen = 0;
  memset(req->headers, 0x0, sizeof(req->headers));

  req->paramsLen = 0;
  memset(req->params, 0x0, sizeof(req->params));

  req->bodyLen = 0;
  req->contentLength = 0;

  req->argsLen = 0;

  req->state = stateMetodoInicio;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus http_onFormat(int clientFd, BuffReader *reader) {
  HttpClient *client = http_client(clientFd);
  const char *c = NULL;
  size_t r;

  while ((r = buff_reader_read(reader, &c, 1)) > 0) {
    FormatStatus state = client->req->state(client, *c);

    if (state != FORMAT_PART) {
      return state;
    }
  }

  return FORMAT_PART;
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

static FormatStatus stateMetodoInicio(HttpClient *client, char c) {
  if (c < 'A' || c > 'Z') {
    log_erro("http",
             "stateMetodoInicio() - esperando a letra (A-Z), mas veio: %c.\n",
             c);
    return FORMAT_ERROR;
  }

  client->req->state = stateMetodo;

  client->req->method[client->req->methodLen++] = c;
  client->req->method[client->req->methodLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateMetodo(HttpClient *client, char c) {
  if (c == ' ') {
    log_dbug("http", "Método: %s\n", client->req->method);
    client->req->state = stateUri;
    return FORMAT_PART;
  }

  if (c < 'A' || c > 'Z') {
    log_erro("http", "stateMetodo() - esperando a letra (A-Z), mas veio: %c.\n",
             c);
    return FORMAT_ERROR;
  }

  client->req->method[client->req->methodLen++] = c;
  client->req->method[client->req->methodLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateUri(HttpClient *client, char c) {
  if (c == ' ') {
    log_dbug("http", "Uri: %s\n", client->req->uri);
    client->req->state = stateHttpVersionH;
    return FORMAT_PART;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateUri() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return FORMAT_ERROR;
  }

  if (c == '?') {
    client->req->state = stateParamNovo;
    return FORMAT_PART;
  }

  client->req->uri[client->req->uriLen++] = c;
  client->req->uri[client->req->uriLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateParamNome(HttpClient *client, char c) {
  HttpParam *param = &client->req->params[client->req->paramsLen - 1];

  if (c == ' ') {
    log_dbug("http", "Nome de parâmetro: %s\n", param->name);
    client->req->state = stateHttpVersionH;
    return FORMAT_PART;
  }

  if (c == '=') {
    log_dbug("http", "Nome de parâmetro: %s\n", param->name);
    client->req->state = stateParamValor;
    return FORMAT_PART;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamNome() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return FORMAT_ERROR;
  }

  if (param->nameLen >= PARAM_NAME_MAX) {
    log_erro("http",
             "stateParamNome() - nome do parâmetro maior do que o permitido: "
             "%ld\n",
             PARAM_NAME_MAX);
    return FORMAT_ERROR;
  }

  param->name[param->nameLen++] = c;
  param->name[param->nameLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateParamValor(HttpClient *client, char c) {
  HttpParam *param = &client->req->params[client->req->paramsLen - 1];

  if (c == ' ') {
    log_dbug("http", "Valor de parâmetro: %s\n", param->value);
    client->req->state = stateHttpVersionH;
    return FORMAT_PART;
  }

  if (c == '&') {
    log_dbug("http", "Valor de parâmetro: %s\n", param->value);
    client->req->state = stateParamNovo;
    return FORMAT_PART;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamValor() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return FORMAT_ERROR;
  }

  if (param->valueLen >= PARAM_VALUE_MAX) {
    log_erro("http",
             "stateParamValor() - valor do parâmetro maior do que o "
             "permitido: %ld.\n",
             PARAM_VALUE_MAX);
    return FORMAT_ERROR;
  }

  param->value[param->valueLen++] = c;
  param->value[param->valueLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateParamNovo(HttpClient *client, char c) {
  if (http_isCtl(c)) {
    log_erro("http",
             "stateParamNovo() - esperando a letra, número, símbolo ou "
             "pontuação, mas veio: %02X.\n",
             c);
    return FORMAT_ERROR;
  }

  if (client->req->paramsLen >= PARAMS_MAX) {
    log_erro("http",
             "stateParamNovo() - quantidade de parâmetros maior do que o "
             "permitido: %ld\n",
             PARAMS_MAX);
    return FORMAT_ERROR;
  }

  log_dbug("http", "stateParamNovo()\n");

  HttpParam *param = &client->req->params[client->req->paramsLen++];
  param->name[param->nameLen++] = c;
  param->name[param->nameLen] = '\0';

  client->req->state = stateParamNome;
  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersionH(HttpClient *client, char c) {
  if (c == 'H') {
    client->req->state = stateHttpVersionT1;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersionH() - esperando a letra 'H', mas veio: %c.\n", c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersionT1(HttpClient *client, char c) {
  if (c == 'T') {
    client->req->state = stateHttpVersionT2;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersionT1() - esperando a letra 'T', mas veio: %c.\n", c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersionT2(HttpClient *client, char c) {
  if (c == 'T') {
    client->req->state = stateHttpVersionP;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersionT2() - esperando a letra 'T', mas veio: %c.\n", c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersionP(HttpClient *client, char c) {
  if (c == 'P') {
    client->req->state = stateHttpVersionSlash;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersionP() - esperando a letra 'P', mas veio: %c.\n", c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersionSlash(HttpClient *client, char c) {
  if (c == '/') {
    client->req->versionMajor = 0;
    client->req->versionMinor = 0;
    client->req->state = stateHttpVersao1Inicio;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersionSlash() - esperando barra invertida (/), mas "
           "veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersao1Inicio(HttpClient *client, char c) {
  if (http_isDigit(c)) {
    client->req->versionMajor *= 10 + (unsigned)c - '0';
    client->req->state = stateHttpVersao1;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersao1Inicio() - esperando dígito (0-9), mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersao1(HttpClient *client, char c) {
  if (c == '.') {
    client->req->state = stateHttpVersao2Inicio;
    return FORMAT_PART;
  } else if (http_isDigit(c)) {
    client->req->versionMajor *= 10 + (unsigned)c - '0';
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersao1() - esperando sinal de ponto-final (.) ou "
           "dígito (0-9), mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersao2Inicio(HttpClient *client, char c) {
  if (http_isDigit(c)) {
    client->req->versionMinor *= 10 + (unsigned)c - '0';
    client->req->state = stateHttpVersao2;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersao2Inicio() - esperando dígito (0-9), mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateHttpVersao2(HttpClient *client, char c) {
  if (c == '\r') {
    client->req->state = stateNovaLinha1;
    return FORMAT_PART;
  } else if (http_isDigit(c)) {
    client->req->versionMinor *= 10 + (unsigned)c - '0';
    return FORMAT_PART;
  }

  log_erro("http",
           "stateHttpVersao2() - esperando retorno de cursor '\\r' ou dígito "
           "(0-9), mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateNovaLinha1(HttpClient *client, char c) {
  if (c == '\n') {
    client->req->state = stateCabecalhoNovo;
    return FORMAT_PART;
  }

  log_erro("http", "stateNovaLinha1() - esperando nova linha, mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateCabecalhoNovo(HttpClient *client, char c) {
  if (c == '\r') {
    client->req->state = stateNovaLinha3;
    return FORMAT_PART;
  }

  if (!http_isLetter(c)) {
    log_erro("http", "stateCabecalhoNovo() - esperando letra, mas veio: %c.\n",
             c);
    return FORMAT_ERROR;
  }

  if (client->req->headersLen >= HEADERS_MAX) {
    log_erro("http",
             "stateCabecalhoNovo() - quantidade de cabecalhos maior do que o "
             "permitido: %ld.\n",
             HEADERS_MAX);
    return FORMAT_ERROR;
  }

  HttpHeader *header = &client->req->headers[client->req->headersLen++];
  header->name[header->nameLen++] = c;
  header->name[header->nameLen] = '\0';

  client->req->state = stateCabecalhoNome;
  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateCabecalhoEspacoAntesDoValor(HttpClient *client,
                                                     char c) {
  if (c == ' ') {
    client->req->state = stateCabecalhoValor;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateCabecalhoEspacoAntesDoValor() - esperando espaço em branco, "
           "mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateCabecalhoNome(HttpClient *client, char c) {
  HttpHeader *header = &client->req->headers[client->req->headersLen - 1];

  if (c == ':') {
    log_dbug("http", "Nome de cabeçalho: %s\n", header->name);
    client->req->state = stateCabecalhoEspacoAntesDoValor;
    return FORMAT_PART;
  }

  if (!http_isLetter(c) && c != '-') {
    log_erro("http",
             "stateCabecalhoNome() - esperando letra ou traço, mas "
             "veio: %c.\n",
             c);
    return FORMAT_ERROR;
  }

  if (header->nameLen >= HEADER_NAME_MAX) {
    log_erro("http",
             "stateCabecalhoNome() - nome do cabeçalho maior do que o "
             "permitido: %ld.\n",
             HEADER_NAME_MAX);
    return FORMAT_ERROR;
  }

  header->name[header->nameLen++] = c;
  header->name[header->nameLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateCabecalhoValor(HttpClient *client, char c) {
  HttpHeader *header = &client->req->headers[client->req->headersLen - 1];

  if (c == '\r') {
    log_dbug("http", "Valor de cabeçalho: %s\n", header->value);
    client->req->state = stateNovaLinha2;
    return FORMAT_PART;
  }

  if (http_isCtl(c)) {
    log_erro("http",
             "stateCabecalhoValor() - esperando letra, número ou símbolo, "
             "mas veio caractere de controle: %02X.\n",
             c);
    return FORMAT_ERROR;
  }

  if (header->valueLen >= HEADER_VALUE_MAX) {
    log_erro("http",
             "stateCabecalhoValor() - valor do cabeçalho maior do que o "
             "permitido: %ld.\n",
             HEADER_VALUE_MAX);
    return FORMAT_ERROR;
  }

  header->value[header->valueLen++] = c;
  header->value[header->valueLen] = '\0';

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateNovaLinha2(HttpClient *client, char c) {
  if (c == '\n') {
    client->req->state = stateCabecalhoNovo;
    return FORMAT_PART;
  }

  log_erro("http",
           "stateNovaLinha2() - esperando nova linha '\\n', mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateNovaLinha3(HttpClient *client, char c) {
  if (c == '\n') {
    const char *contentLength = http_reqHeader(client->fd, "Content-Length");

    if (contentLength != NULL) {
      client->req->contentLength = atoi(contentLength);

      if (client->req->contentLength >= BODY_MAX) {
        return FORMAT_ERROR;
      }

      if (client->req->contentLength > 0) {
        client->req->state = stateBody;
        return FORMAT_PART;
      }
    }

    return FORMAT_OK;
  }

  log_erro("http",
           "stateNovaLinha3() - esperando nova linha '\\n', mas veio: %c.\n",
           c);

  return FORMAT_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus stateBody(HttpClient *client, char c) {
  if (client->req->bodyLen < client->req->contentLength) {
    client->req->body[client->req->bodyLen++] = c;
    client->req->body[client->req->bodyLen] = 0;
  }

  if (client->req->bodyLen == client->req->contentLength) {
    return FORMAT_OK;
  }

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

void http_sendNotFound(int clientFd) {
  http_sendStatus(clientFd, HTTP_STATUS_NOT_FOUND);
  http_send(clientFd, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////

void http_sendError(int clientFd) {
  http_sendStatus(clientFd, HTTP_STATUS_INTERNAL_ERROR);
  http_send(clientFd, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////

void http_sendRedirect(int clientFd, const char *url) {
  http_sendStatus(clientFd, HTTP_STATUS_MOVED_PERMANENTLY);
  http_sendHeader(clientFd, "Location", url);
  http_send(clientFd, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////

void http_sendStatus(int clientFd, HttpStatus status) {
  HttpClient *client = http_client(clientFd);
  str_fmt(client->resp, "HTTP/1.1 %d %s\r\n", status, http_strStatus(status));
}

////////////////////////////////////////////////////////////////////////////////

void http_sendType(int clientFd, HttpMimeType type) {
  HttpClient *client = http_client(clientFd);
  str_fmt(client->resp, "Content-Type: %s\r\n", http_strMimeType(type));
}

////////////////////////////////////////////////////////////////////////////////

void http_sendHeader(int clientFd, const char *name, const char *value) {
  HttpClient *client = http_client(clientFd);
  str_fmt(client->resp, "%s: %s\r\n", name, value);
}

////////////////////////////////////////////////////////////////////////////////

void http_sendHeaderInt(int clientFd, const char *name, int value) {
  HttpClient *client = http_client(clientFd);
  str_fmt(client->resp, "%s: %d\r\n", name, value);
}

////////////////////////////////////////////////////////////////////////////////

void http_send(int clientFd, const char *body, size_t size) {
  HttpClient *client = http_client(clientFd);

  if (body == NULL || size == 0) {
    http_sendHeader(clientFd, "Content-Length", "0");
    str_addcstr(&client->resp, "\r\n");
  } else {
    http_sendHeaderInt(clientFd, "Content-Length", size);
    str_addcstr(&client->resp, "\r\n");
    str_addcstrlen(&client->resp, body, size);
  }

  log_dbug("http", "<<< %s\n", str_cstr(client->resp));

  server_send(clientFd, str_cstr(client->resp), str_len(client->resp));
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
    case HTTP_STATUS_INTERNAL_ERROR:
      return "Internal Error";
    case HTTP_STATUS_MOVED_PERMANENTLY:
      return "Moved Permanently";
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static const char *http_strMimeType(HttpMimeType contentType) {
  switch (contentType) {
    case HTTP_TYPE_HTML:
      return "text/html; charset=utf8";
    case HTTP_TYPE_JSON:
      return "application/json; charset=utf8";
    case HTTP_TYPE_TEXT:
      return "text/plain; charset=utf8";
    case HTTP_TYPE_CSS:
      return "text/css; charset=utf8";
    case HTTP_TYPE_JS:
      return "application/javascript; charset=utf8";
    case HTTP_TYPE_JPEG:
      return "image/jpeg; charset=utf8";
    case HTTP_TYPE_PNG:
      return "image/png; charset=utf8";
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqHeader(int clientFd, const char *name) {
  HttpClient *client = http_client(clientFd);
  for (int i = 0; i < client->req->headersLen; i++) {
    if (strcmp(client->req->headers[i].name, name) == 0) {
      return client->req->headers[i].value;
    }
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqParam(int clientFd, const char *name) {
  HttpClient *client = http_client(clientFd);
  for (int i = 0; i < client->req->paramsLen; i++) {
    if (strcmp(client->req->params[i].name, name) == 0) {
      return client->req->params[i].value;
    }
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////

int http_reqParamInt(int clientFd, const char *name, int def) {
  HttpClient *client = http_client(clientFd);
  for (int i = 0; i < client->req->paramsLen; i++) {
    if (strcmp(client->req->params[i].name, name) == 0) {
      int r = strtol(client->req->params[i].value, NULL, 10);
      if ((r == LONG_MAX || r == LONG_MIN) && errno == ERANGE) {
        return def;
      } else {
        return r;
      }
    }
  }
  return def;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqMethod(int clientFd) {
  HttpClient *client = http_client(clientFd);
  return client->req->method;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqPath(int clientFd) {
  HttpClient *client = http_client(clientFd);
  return client->req->uri;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqPattern(int clientFd) {
  HttpClient *client = http_client(clientFd);
  return client->req->pattern;
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqArg(int clientFd, int n) {
  HttpClient *client = http_client(clientFd);
  if (n >= client->req->argsLen) {
    return "";
  }
  return client->req->args[n];
}

////////////////////////////////////////////////////////////////////////////////

const char *http_reqBody(int clientFd) {
  HttpClient *client = http_client(clientFd);
  return client->req->body;
}