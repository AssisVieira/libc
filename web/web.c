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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "assets/assets.h"
#include "db/db.h"
#include "io/io.h"
#include "log/log.h"

#define IO_MAX_EVENTS 512

////////////////////////////////////////////////////////////////////////////////

void web_handler(const char *method, const char *path, WebHandler handler) {
  http_handler(method, path, handler);
}

////////////////////////////////////////////////////////////////////////////////

int web_start() {
  int r = 0;

  // Startup...

  log_ignore("tcp", LOG_INFO);
  log_ignore("tcp-inbox", LOG_INFO);
  log_ignore("tcp-outbox", LOG_INFO);
  log_ignore("tcp-port", LOG_INFO);
  log_ignore("io", LOG_INFO);
  log_ignore("db", LOG_INFO);
  log_ignore("http", LOG_TRAC);

  if (db_openPool(5, 10)) {
    perror("db_openPool()\n");
    return -1;
  }

  if (http_open("2000", 100)) {
    log_erro("http", "http_open().\n");
    return -1;
  }

  // Execution...

  if (io_run(IO_MAX_EVENTS)) {
    log_erro("http", "io_run().\n");
    r = -1;
  }

  // Shutdown...

  if (http_close()) {
    log_erro("http", "http_close().\n");
    r = -1;
  }

  db_closePool();

  return r;
}

////////////////////////////////////////////////////////////////////////////////

void web_close(int result) { io_close(result); }

////////////////////////////////////////////////////////////////////////////////

void web_assets(const char *from, const char *to) {}

////////////////////////////////////////////////////////////////////////////////

void web_redirect(const char *from, const char *to) {}

////////////////////////////////////////////////////////////////////////////////

// static HttpMimeType mimeTypeByFilename(const char *filename) {
//   const char *extensao = strchr(filename, '.');

//   if (extensao == NULL) return HTTP_TYPE_TEXT;

//   if (strcmp(extensao, ".html") == 0 || strcmp(extensao, ".HTML") == 0)
//     return HTTP_TYPE_HTML;

//   if (strcmp(extensao, ".js") == 0 || strcmp(extensao, ".JS") == 0)
//     return HTTP_TYPE_JS;

//   if (strcmp(extensao, ".css") == 0 || strcmp(extensao, ".CSS") == 0)
//     return HTTP_TYPE_CSS;

//   if (strcmp(extensao, ".jpg") == 0 || strcmp(extensao, ".JPG") == 0)
//     return HTTP_TYPE_JPEG;

//   if (strcmp(extensao, ".png") == 0 || strcmp(extensao, ".PNG") == 0)
//     return HTTP_TYPE_PNG;

//   return HTTP_TYPE_TEXT;
// }
