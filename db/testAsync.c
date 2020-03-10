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
#include <assert.h>

static void insertPeople(DB *db);
static void selectPeople(DB *db);
static void onPeopleSelected(DB *db);

typedef struct People {
  char name[32];
  char email[128];
} People;

int main() {
  IO *io = io_new();

  if (db_openPool(5, 10, true)) {
    perror("Falha ao abrir o pool.\n");
    return -1;
  }

  DB *db = db_open();

  assert(db != NULL);

  insertPeople(db);

  return io_run(io, 10);
}

static void insertPeople(DB *db) {
  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, "John");
  db_param(db, "john@john.com");
  db_send(db, NULL, selectPeople);
}

static void selectPeople(DB *db) {
  assert(db_error(db) == false);
  db_sql(db, "select name, email from people limit 1");
  db_send(db, NULL, onPeopleSelected);
}

static void onPeopleSelected(DB *db) {
  assert(db_error(db) == false);
  assert( db_count(db) == 1 );
  assert( strcmp(db_value(db, 0, 0), "John") == 0 );
  assert( strcmp(db_value(db, 0, 1), "john@john.com") == 0 );
  db_close(db);
  io_close(io_current(), 0);
}
