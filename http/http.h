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

#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

////////////////////////////////////////////////////////////////////////////////

#define HANDLERS_MAX 100

////////////////////////////////////////////////////////////////////////////////

#define HEADER_NAME_MAX 128
#define HEADER_VALUE_MAX 512
#define HEADERS_MAX 32

#define HTTP_RESP_INIT_SIZE (4 * 1024)

#define HTTP_CLIENTS_INIT_SIZE 32

////////////////////////////////////////////////////////////////////////////////

#define PARAM_NAME_MAX 128
#define PARAM_VALUE_MAX 128
#define PARAMS_MAX 32

////////////////////////////////////////////////////////////////////////////////

#define URI_MAX 256
#define METHOD_MAX 8
#define PATTERN_MAX URI_MAX

////////////////////////////////////////////////////////////////////////////////

#define INBOX_MAX_SIZE (4 * 1024)
#define OUTBOX_INIT_SIZE (4 * 1024)

////////////////////////////////////////////////////////////////////////////////

#define ARGS_MAX 4
#define ARG_MAX URI_MAX

////////////////////////////////////////////////////////////////////////////////

#define BODY_MAX (6 * 1024)

////////////////////////////////////////////////////////////////////////////////

typedef struct HttpClient HttpClient;

////////////////////////////////////////////////////////////////////////////////

typedef enum HttpMimeType {
  HTTP_TYPE_JSON,
  HTTP_TYPE_HTML,
  HTTP_TYPE_JPEG,
  HTTP_TYPE_PNG,
  HTTP_TYPE_CSS,
  HTTP_TYPE_JS,
  HTTP_TYPE_TEXT,
} HttpMimeType;

////////////////////////////////////////////////////////////////////////////////

typedef enum HttpStatus {
  HTTP_STATUS_OK = 200,
  HTTP_STATUS_NOT_FOUND = 404,
  HTTP_STATUS_BAD_REQUEST = 400,
  HTTP_STATUS_INTERNAL_ERROR = 500,
  HTTP_STATUS_MOVED_PERMANENTLY = 301,
} HttpStatus;

////////////////////////////////////////////////////////////////////////////////

typedef void (*HttpHandlerFunc)(int clientFd);

////////////////////////////////////////////////////////////////////////////////
// STARTUP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

int http_start(int port, int maxClients);

int http_handler(const char *method, const char *path, HttpHandlerFunc func);

void http_stop(int result);

////////////////////////////////////////////////////////////////////////////////
// REQUEST FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

const char *http_reqHeader(int clientFd, const char *name);

const char *http_reqParam(int clientFd, const char *name);

int http_reqParamInt(int clientFd, const char *name, int def);

const char *http_reqMethod(int clientFd);

const char *http_reqPath(int clientFd);

const char *http_reqPattern(int clientFd);

const char *http_reqArg(int clientFd, int n);

const char *http_reqBody(int clientFd);

////////////////////////////////////////////////////////////////////////////////
// RESPONSE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void http_sendNotFound(int clientFd);

void http_sendError(int clientFd);

void http_sendRedirect(int clientFd, const char *url);

void http_sendStatus(int clientFd, HttpStatus status);

void http_sendType(int clientFd, HttpMimeType mimeType);

void http_sendHeader(int clientFd, const char *nome, const char *valor);

void http_sendHeaderInt(int clientFd, const char *nome, int valor);

void http_send(int clientFd, const char *body, size_t size);

#endif
