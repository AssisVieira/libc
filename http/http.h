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

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stddef.h>

////////////////////////////////////////////////////////////////////////////////

#define HANDLERS_MAX 100

////////////////////////////////////////////////////////////////////////////////

#define HEADER_NAME_MAX 128
#define HEADER_VALUE_MAX 512
#define HEADERS_MAX 32

////////////////////////////////////////////////////////////////////////////////

#define PARAM_NAME_MAX 128
#define PARAM_VALUE_MAX 128
#define PARAMS_MAX 32

////////////////////////////////////////////////////////////////////////////////

#define URI_MAX 256
#define METHOD_MAX 8

////////////////////////////////////////////////////////////////////////////////

#define TCP_INBOX_SIZE (4 * 1024)
#define TCP_OUTBOX_MAX_SIZE (512 * 1024)

////////////////////////////////////////////////////////////////////////////////

#define ARGS_MAX 4
#define ARG_MAX URI_MAX

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
} HttpStatus;

typedef void (*HttpHandlerFunc)(HttpClient *client);

////////////////////////////////////////////////////////////////////////////////
// STARTUP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

int http_open(const char *port, size_t maxClients);

int http_handler(const char *method, const char *path, HttpHandlerFunc func);

int http_close();

////////////////////////////////////////////////////////////////////////////////
// REQUEST FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

const char *http_reqHeader(const HttpClient *client, const char *name);

const char *http_reqParam(const HttpClient *client, const char *name);

const char *http_reqMethod(const HttpClient *client);

const char *http_reqPath(const HttpClient *client);

const char *http_reqArg(const HttpClient *client, int n);

////////////////////////////////////////////////////////////////////////////////
// RESPONSE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void http_respNotFound(HttpClient *client);

void http_respError(HttpClient *client);

void http_respOk(HttpClient *client, HttpMimeType mimeType, const char *str);

void http_respBegin(HttpClient *client, HttpStatus status,
                    HttpMimeType mimeType);

void http_respHeader(HttpClient *client, const char *nome, const char *valor);

void http_respHeaderInt(HttpClient *client, const char *nome, int valor);

void http_respBody(HttpClient *client, const char *fmt, ...);

void http_respBodyLen(HttpClient *client, const char *buff, size_t len);

int http_respEnd(HttpClient *client);

#endif
