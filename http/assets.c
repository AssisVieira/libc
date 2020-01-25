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

#include "assets.h"
#include "log/log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PATH_MAX_SIZE 2048

typedef struct HttpAssets {
  char dirPublico[PATH_MAX_SIZE];
} HttpAssets;

static HttpAssets assets = {"./web/"};

////////////////////////////////////////////////////////////////////////////////

static char *makePath(const char *dir, const char *nome);
static HttpMimeType mimeTypeByFilename(const char *filename);

////////////////////////////////////////////////////////////////////////////////

int httpAssets_init(const char *dir) {
  assets.dirPublico[0] = 0;
  strncat(assets.dirPublico, dir, sizeof(assets.dirPublico));
  log_info("http-assets", "Inicializando...diretório público: %s.\n",
           assets.dirPublico);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static bool ehArquivo(const char *endereco) {
  struct stat st_buf;
  int status = stat(endereco, &st_buf);

  if (status != 0) {
    log_erro("http-assets", "ehArquivo(): %d - %s\n", errno, strerror(errno));
    return false;
  }

  if (S_ISREG(st_buf.st_mode))
    return true;

  return false;
}

int arquivo_abrir(const char *endereco) {
  if (!ehArquivo(endereco)) {
    log_erro("http-assets",
             "O endereço não se refere a um arquivo convencional: %s.\n",
             endereco);
    return -1;
  }

  return open(endereco, O_RDONLY);
}

////////////////////////////////////////////////////////////////////////////////

void httpAssets_getFile(HttpClient *client) {
  const char *uriPath = http_reqArg(client, 0);

  if (uriPath == NULL || *uriPath == '\0') {
    log_info("http-assets",
             "Arquivo não especificado. Buscando pelo /index.html.\n");
    uriPath = "index.html";
  }

  char *filePath = makePath(assets.dirPublico, uriPath);
  int fd = arquivo_abrir(filePath);

  if (fd < 0) {
    http_respNotFound(client, HTTP_TYPE_HTML, "");
    return;
  }

  http_respBegin(client, HTTP_STATUS_OK, mimeTypeByFilename(filePath));

  char buff[4 * 1024];
  size_t buffSize = sizeof(buff);
  ssize_t buffLen;

  while ((buffLen = read(fd, buff, buffSize)) > 0) {
    http_respBodyLen(client, buff, buffLen);
  }

  if (buffLen == 0) {
    http_respEnd(client);
  } else {
    // TODO Tratar erro.
    http_respEnd(client);
  }

  close(fd);
}

////////////////////////////////////////////////////////////////////////////////
// CONSTRUIR ENDEREÇO DO ARQUIVO A SER LIDO
////////////////////////////////////////////////////////////////////////////////

static char *makePath(const char *dir, const char *nome) {
  size_t enderecoTam;

  enderecoTam = strlen(dir);
  enderecoTam += 1; // divisor /
  enderecoTam += strlen(nome);
  enderecoTam += 1; // byte nulo

  char *endereco = malloc(sizeof(char) * enderecoTam);

  strcpy(endereco, dir);
  strcat(endereco, "/");
  strcat(endereco, nome);

  return endereco;
}

////////////////////////////////////////////////////////////////////////////////

static HttpMimeType mimeTypeByFilename(const char *filename) {
  const char *extensao = strchr(filename, '.');

  if (extensao == NULL)
    return HTTP_TYPE_TEXT;

  if (strcasecmp(extensao, ".html") == 0)
    return HTTP_TYPE_HTML;

  if (strcasecmp(extensao, ".js") == 0)
    return HTTP_TYPE_JS;

  if (strcasecmp(extensao, ".css") == 0)
    return HTTP_TYPE_CSS;

  if (strcasecmp(extensao, ".jpg") == 0)
    return HTTP_TYPE_JPEG;

  if (strcasecmp(extensao, ".png") == 0)
    return HTTP_TYPE_PNG;

  return HTTP_TYPE_TEXT;
}
