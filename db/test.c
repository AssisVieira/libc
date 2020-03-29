#include <stdio.h>
#include <threads.h>

#include "db.h"
#include "log/log.h"

static void testOpen() {
  // db_open() deve retornar NULL, caso a string de conexão for nula.
  assert(db_open(NULL, false) == NULL);

  // db_open() deve retornar uma instância de DB, caso uma conexão for realizada
  // com o banco.
  DB *db = db_open("postgresql://assis:assis@127.0.0.1:5432/assis", false);
  assert(db != NULL);
  db_close(db);
}

static int testConcurrencyWorker(void *arg) {
  DBPool *pool = arg;

  for (int i = 0; i < 100000; i++) {
    DB *db = db_pool_get(pool);
    assert(db != NULL);
    db_sql(db, "select id from people limit 1");
    assert(db_exec(db) == 0);
    db_close(db);
  }

  return 0;
}

static void testConcurrency() {
  thrd_t thread1;
  thrd_t thread2;
  thrd_t thread3;
  DBPool *pool = db_pool_create("postgresql://assis:assis@127.0.0.1:5432/assis",
                                false, 1, 3);

  assert(db_pool_num_busy(pool) == 0);
  assert(db_pool_num_idle(pool) == 1);

  thrd_create(&thread1, testConcurrencyWorker, pool);
  thrd_create(&thread2, testConcurrencyWorker, pool);
  thrd_create(&thread3, testConcurrencyWorker, pool);

  thrd_join(thread1, NULL);
  thrd_join(thread2, NULL);
  thrd_join(thread3, NULL);

  assert(db_pool_num_busy(pool) == 0);
  assert(db_pool_num_idle(pool) == 3);

  db_pool_destroy(pool);
}

int main() {
  log_ignore("db", LOG_DBUG);

  testOpen();

  testConcurrency();

  return 0;
}
