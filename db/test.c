#include "db.h"
#include <stdio.h>

static void onConnected(void *context);
static void onError(void *context);
static void onQueryResult(DBQuery *query, void *context);

int main() {
  const char *connString =
      "host=127.0.0.1 port=5433 dbname=assis user=assis password=assis";

  ioevent_init(true);

  if (db_open(connString, NULL, onConnected, onError)) {
    perror("error: db_open()\n");
    return -1;
  }

  bool finish = false;

  return ioevent_run(&finish);
}

static void onConnected(void *context) {
  printf("onConnected()\n");

  if (db_query("select name from people;", NULL, onQueryResult)) {
    perror("error: db_query()\n");
  }
}

static void onError(void *context) { printf("onError()\n"); }

static void onQueryResult(DBQuery *query, void *context) {
  if (db_queryIsDone(query)) {
    printf("concluido.\n");
    return;
  }

  if (db_queryIsError(query)) {
    printf("error na query.\n");
    return;
  }

  for (int i = 0; i < db_queryCount(query); i++) {
    const char *name = db_value(query, i, 0);
    printf("%s\n", name);
  }
}
