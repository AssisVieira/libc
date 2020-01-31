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

static void insertPeople(DB *db);
static void selectPeople(DB *db);
static void onPeopleSelected(DB *db);

typedef struct People {
  char name[32];
  char email[128];
} People;

static int total = 0;

int main() {
  ioevent_open();

  // log_ignore("ioevent", LOG_DBUG);

  if (db_openPool(5, 10)) {
    perror("Falha ao abrir o pool.\n");
    return -1;
  }

  DB *db = db_open();

  if (db == NULL) {
    perror("Erro ao obter conexão.\n");
    return -1;
  }

  insertPeople(db);

  return ioevent_run();
}

static void insertPeople(DB *db) {
  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, "John");
  db_param(db, "john@john.com");
  db_send(db, NULL, selectPeople);
}

static void selectPeople(DB *db) {

  if (db_error(db)) {
    perror("Erro ao inserir.\n");
    db_close(db);
    return;
  }

  if (total++ < 3000) {
    insertPeople(db);
    return;
  }

  total = 0;

  db_sql(db, "select id, name, email from people");
  db_send(db, NULL, onPeopleSelected);
}

static void onPeopleSelected(DB *db) {
  printf("onPeopleSelected()\n");

  for (int i = 0; i < db_count(db); i++) {
    const char *id = db_value(db, i, 0);
    const char *name = db_value(db, i, 1);
    const char *email = db_value(db, i, 2);
    printf("%s\t\t%s\t\t%s\n", id, name, email);
  }

  if (total++ < 5) {
    db_sql(db, "select id, name, email from people");
    db_send(db, NULL, onPeopleSelected);
    return;
  }

  db_close(db);
  ioevent_close(0);
}
