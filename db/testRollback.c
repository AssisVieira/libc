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

#include "db.h"
#include "log/log.h"
#include <stdio.h>
#include <string.h>

int main() {
  if (db_openPool(5, 10, false)) {
    perror("Falha ao abrir o pool.\n");
    return -1;
  }

  DB *db = db_open();

  assert(db != NULL);

  db_sql(db, "delete from people");
  db_exec(db);
  db_commit(db);

  db_sql(db, "delete from people");
  assert(db_exec(db) == 0);

  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, "John");
  db_param(db, "john@john.com");
  assert(db_exec(db) == 0);

  db_rollback(db);

  db_sql(db, "select id, name, email from people");
  assert(db_exec(db) == 0);

  assert(db_count(db) == 0);

  db_close(db);

  db_closePool();

  return 0;
}
