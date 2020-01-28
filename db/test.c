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

// export DB_STRING_CONNECTION="host=127.0.0.1 port=5433 user=assis
// password=assis dbname=assis sslmode=require"

static int total = 0;
static bool finish = false;
static char *SQL_SELECT = "select id, name, email from people";
static char *SQL_INSERT =
    "insert into people (name, email) values ($1::text, $2::text)";

int main() {
  ioevent_open();

  log_ignore("ioevent", LOG_DBUG);

  People *people = malloc(sizeof(People));
  strcpy(people->name, "John");
  strcpy(people->email, "john@john.com");

  db_open(people, insertPeople, onConnectionError);

  ioevent_run(&finish);

  ioevent_close();

  return 0;
}

static void insertPeople(DB *db) {
  People *people = db_context(db);
  db_sql(db, SQL_INSERT);
  db_param(db, people->name);
  db_param(db, people->email);
  db_send(db, selectPeople, onSqlError);
}

static void selectPeople(DB *db) {

  if (total++ < 3000) {
    insertPeople(db);
    return;
  }

  db_sql(db, SQL_SELECT);
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

  if (total++ < 3000) {
    db_sql(db, SQL_SELECT);
    db_send(db, onPeopleSelected, onSqlError);
    return;
  }

  free(db_context(db));
  db_close(db);
  finish = true;
}

static void onConnectionError(DB *db) {
  printf("onConnectionError()\n");
  free(db_context(db));
  db_close(db);
  finish = true;
}

static void onSqlError(DB *db) {
  printf("onSqlError()\n");
  free(db_context(db));
  db_close(db);
  finish = true;
}
