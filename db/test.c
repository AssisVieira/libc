#include "db.h"
#include "log/log.h"
#include <stdio.h>
#include <string.h>

static void insertPeople(DB *db);
static void selectPeople(DB *db);
static void onPeopleSelected(DB *db);
static void onSqlError(DB *db);
static void onConnectionError(DB *db);

typedef struct People {
  char name[32];
  char email[128];
} People;

static int total = 0;

int main() {
  ioevent_open();

  log_ignore("ioevent", LOG_DBUG);

  People *people = malloc(sizeof(People));
  strcpy(people->name, "John");
  strcpy(people->email, "john@john.com");

  db_open(people, insertPeople, onConnectionError);

  return ioevent_run();
}

static void insertPeople(DB *db) {
  People *people = db_context(db);
  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, people->name);
  db_param(db, people->email);
  db_send(db, selectPeople, onSqlError);
}

static void selectPeople(DB *db) {

  if (total++ < 3000) {
    insertPeople(db);
    return;
  }

  total = 0;

  db_sql(db, "select id, name, email from people");
  db_send(db, onPeopleSelected, onSqlError);
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
    db_send(db, onPeopleSelected, onSqlError);
    return;
  }

  free(db_context(db));
  db_close(db);
  ioevent_close(0);
}

static void onConnectionError(DB *db) {
  printf("onConnectionError()\n");
  free(db_context(db));
  db_close(db);
  ioevent_close(-1);
}

static void onSqlError(DB *db) {
  printf("onSqlError()\n");
  free(db_context(db));
  db_close(db);
  ioevent_close(-1);
}
