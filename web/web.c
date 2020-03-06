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

#include "web.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#include "assets/assets.h"
#include "db/db.h"
#include "hashTable/hashTable.h"
#include "http/http.h"
#include "io/io.h"
#include "log/log.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct Web {
  HashTable *redirects;
  char publicDir[PATH_MAX];
} Web;

////////////////////////////////////////////////////////////////////////////////

static thread_local Web web = {
    .redirects = NULL,
    .publicDir = {0},
};

////////////////////////////////////////////////////////////////////////////////

static HttpMimeType mimeTypeByFilename(const char *filename);
static void web_redirectHandler(HttpClient *client);
static void web_assetHandler(HttpClient *client);
static int web_init();

////////////////////////////////////////////////////////////////////////////////

int web_start(int port, int maxClients) {
  // Startup...

  log_ignore("tcp", LOG_INFO);
  log_ignore("tcp-inbox", LOG_INFO);
  log_ignore("tcp-outbox", LOG_INFO);
  log_ignore("tcp-port", LOG_INFO);
  log_ignore("io", LOG_INFO);
  log_ignore("db", LOG_INFO);
  log_ignore("http", LOG_TRAC);

  if (web_init()) {
    return -1;
  }

  if (db_openPool(5, 10)) {
    perror("db_openPool()\n");
    return -1;
  }

  // Execution...

  int r = http_start(port, maxClients);

  // Shutdown...

  db_closePool();

  hashTable_free(web.redirects);
  free(web.redirects);

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static int web_init() {
  if (web.redirects != NULL) return 0;

  web.redirects = malloc(sizeof(HashTable));

  if (web.redirects == NULL) {
    log_erro("web", "malloc(): %d - %s\n", errno, strerror(errno));
    return -1;
  }

  if (hashTable_init(web.redirects, 50)) {
    free(web.redirects);
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void web_handler(const char *method, const char *path, WebHandler handler) {
  http_handler(method, path, handler);
}

////////////////////////////////////////////////////////////////////////////////

void web_close(int result) { http_stop(result); }

////////////////////////////////////////////////////////////////////////////////

void web_assets(const char *pattern, const char *publicDir) {
  assets_open(publicDir);
  strcpy(web.publicDir, publicDir);
  web_handler("GET", pattern, web_assetHandler);
}

////////////////////////////////////////////////////////////////////////////////

static void web_assetHandler(HttpClient *client) {
  const char *pathReq = http_reqArg(client, 0);
  char pathFile[PATH_MAX];

  strcpy(pathFile, web.publicDir);

  if (web.publicDir[strlen(web.publicDir) - 1] == '/' && pathReq[0] == '/') {
    strcat(pathFile, pathReq + 1);
  } else {
    strcat(pathFile, pathReq);
  }

  log_dbug("web", "Asset: %s\n", pathFile);

  if (assets_isDir(pathFile)) {
    strcat(pathFile, "/index.html");
  }

  if (!assets_exists(pathFile)) {
    http_sendNotFound(client);
    return;
  }

  http_sendStatus(client, HTTP_STATUS_OK);
  http_sendType(client, mimeTypeByFilename(pathFile));
  http_send(client, assets_get(pathFile), assets_size(pathFile));
}

////////////////////////////////////////////////////////////////////////////////

void web_redirect(const char *pattern, const char *to) {
  web_init();
  hashTable_set(web.redirects, pattern, (void *)to);
  web_handler("GET", pattern, web_redirectHandler);
}

////////////////////////////////////////////////////////////////////////////////

static void web_redirectHandler(HttpClient *client) {
  const char *pattern = http_reqPattern(client);
  const char *pathOrig = http_reqPath(client);
  const char *pathDest = hashTable_value(web.redirects, pattern);
  log_dbug("web", "Redirecting %s -> %s\n", pathOrig, pathDest);
  http_sendRedirect(client, pathDest);
}

////////////////////////////////////////////////////////////////////////////////

static HttpMimeType mimeTypeByFilename(const char *filename) {
  const char *extensao = strchr(filename, '.');

  if (extensao == NULL) return HTTP_TYPE_TEXT;

  if (strcmp(extensao, ".html") == 0 || strcmp(extensao, ".HTML") == 0)
    return HTTP_TYPE_HTML;

  if (strcmp(extensao, ".js") == 0 || strcmp(extensao, ".JS") == 0)
    return HTTP_TYPE_JS;

  if (strcmp(extensao, ".css") == 0 || strcmp(extensao, ".CSS") == 0)
    return HTTP_TYPE_CSS;

  if (strcmp(extensao, ".jpg") == 0 || strcmp(extensao, ".JPG") == 0)
    return HTTP_TYPE_JPEG;

  if (strcmp(extensao, ".png") == 0 || strcmp(extensao, ".PNG") == 0)
    return HTTP_TYPE_PNG;

  return HTTP_TYPE_TEXT;
}
