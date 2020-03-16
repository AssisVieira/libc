#include "actors.h"
#include <stdio.h>
#include <string.h>
#include "db/db.h"

enum {
  ACTOR_A_MSG_ADD,
  ACTOR_B_MSG_LIST,
};

typedef struct User {
  char name[128];
  char email[128];
} User;

////////////////////////////////////////////////////////////////////////////////

static void actorA_handle(int type, const void* msg, int size) {
  if (type == ACTOR_A_MSG_ADD) {
    const User* user = msg;
    DB* db = db_open();
    db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
    db_param(db, user->name);
    db_param(db, user->email);
    if (db_exec(db)) {
      printf("[actor-a] Fail\n");
    }
    db_close(db);
    return;
  }
}

static int actorA_create(Actors* actors, const char* name) {
  return actors_actorCreate(actors, name, 100, actorA_handle);
}

////////////////////////////////////////////////////////////////////////////////

static void actorB_handle(int type, const void* msg, int size) {
  if (type == ACTOR_B_MSG_LIST) {
    DB* db = db_open();
    db_sql(db, "select id, name, email from people order by id desc limit 1");
    if (db_exec(db) && db_count(db) == 1) {
      printf("[actor-b] Fail\n");
    }
    printf("{id: %s, name: %s, email: %s}\n", db_value(db, 0, 0),
           db_value(db, 0, 1), db_value(db, 0, 2));
    db_close(db);
    return;
  }
}

static int actorB_create(Actors* actors, const char* name) {
  return actors_actorCreate(actors, name, 100, actorB_handle);
}

////////////////////////////////////////////////////////////////////////////////

int main() {
  Actors* actors = actors_create(4);

  db_openPool(50, 50, false);

  int actorA1 = actorA_create(actors, "actorA1", pool);

  int actorB1 = actorB_create(actors, "actorB1", pool);

  for (int i = 0; i < 1000000; i++) {
    User user = {"Assis Vieira", "assis.sv@gmail.com"};
    snprintf(user.name, sizeof(user.name), "Assis Vieira %d", i);
    actors_send(actors, actorA1, ACTOR_A_MSG_ADD, &user, sizeof(User));
    actors_send(actors, actorB1, ACTOR_B_MSG_LIST, NULL, 0);
  }

  db_closePool();

  return 0;
}

// esquizofrenoias - fausto fanti