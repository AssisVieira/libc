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
#include <stdio.h>

static void onInserted(DB *db);

int main() {
  const char strConn[] = "postgresql://assis:assis@127.0.0.1:5432/assis";
  IO *io = io_new();

  DB *db = db_open(strConn, true);

  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, "John");
  db_param(db, "john@john.com");
  db_send(db, io, onInserted);

  return io_run(io, 10);
}

static void onInserted(DB *db) {
  IO *io = db_context(db);

  if (db_error(db)) {
    perror("error on insert.\n");
  } else {
    printf("Row inserted.\n");
  }

  db_close(db);
  io_close(io, 0);
}
