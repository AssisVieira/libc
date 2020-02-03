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

#include "db/db.h"
#include "assets/httpAssets.h"
#include "http/http.h"
#include "httpPeople.h"
#include "log/log.h"
#include <stdbool.h>
#include <stdio.h>

int main() {
  int r = 0;

  log_ignore("tcp", LOG_INFO);
  log_ignore("tcp-inbox", LOG_INFO);
  log_ignore("tcp-outbox", LOG_INFO);
  log_ignore("tcp-port", LOG_INFO);
  log_ignore("ioevent", LOG_INFO);
  log_ignore("http", LOG_TRAC);
  log_ignore("db", LOG_TRAC);

  // Startup...

  if (ioevent_open()) {
    return -1;
  }

  if (db_openPool(5, 10)) {
    perror("db_openPool()\n");
    return -1;
  }

  if (httpAssets_init("http/exemplo/web")) {
    perror("httpAssets_init()\n");
    return -1;
  }

  http_handler("GET", "/web/(.*)", httpAssets_getFile);

  http_handler("GET", "/people/search$", httpPeople_search);

  http_handler("GET", "/people/add$", httpPeople_addForm);

  http_handler("POST", "/people/add$", httpPeople_add);

  if (http_open("2000", 100)) {
    log_erro("http", "http_open().\n");
    return -1;
  }

  // Execution...

  if (ioevent_run()) {
    log_erro("http", "ioevent_run().\n");
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
