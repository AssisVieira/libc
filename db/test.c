#include <stdio.h>
#include <pthread.h>

#include "asserts/asserts.h"
#include "db.h"
#include "log/log.h"

static void testOpen() {
  // db_open() deve retornar NULL, caso a string de conexão for nula.
  assertNull(db_open(NULL, false));

  // db_open() deve retornar uma instância de DB, caso uma conexão for realizada
  // com o banco.
  DB *db = db_open("postgresql://assis:assis@127.0.0.1:5432/assis", false);
  assertNotNull(db);
  db_close(db);
}

static int testPoolCreateGetAndClose() {
  DBPool *pool = db_pool_create("postgresql://assis:assis@127.0.0.1:5432/assis",
                                false, 1, 3);

  assertNotNull(pool);
 
  assertEqual(db_pool_num_busy(pool), 0);
  assertEqual(db_pool_num_idle(pool), 1);

  DB *db1 = db_pool_get(pool);
  assertNotNull(db1);
  assertEqual(db_pool_num_busy(pool), 1);
  assertEqual(db_pool_num_idle(pool), 0);

  DB *db2 = db_pool_get(pool);
  assertNotNull(db2);
  assertEqual(db_pool_num_busy(pool), 2);
  assertEqual(db_pool_num_idle(pool), 0);

  DB *db3 = db_pool_get(pool);
  assertNotNull(db3);
  assertEqual(db_pool_num_busy(pool), 3);
  assertEqual(db_pool_num_idle(pool), 0);

  db_close(db3);
  assertEqual(db_pool_num_busy(pool), 2);
  assertEqual(db_pool_num_idle(pool), 1);

  db_close(db2);
  assertEqual(db_pool_num_busy(pool), 1);
  assertEqual(db_pool_num_idle(pool), 1);

  db_close(db1);
  assertEqual(db_pool_num_busy(pool), 0);
  assertEqual(db_pool_num_idle(pool), 1);

  db_pool_destroy(pool);

  return 0;
}

static void * testConcurrencyWorker(void *arg) {
  DBPool *pool = arg;

  for (int i = 0; i < 1000; i++) {
    DB *db = db_pool_get(pool);
    printf("%d idle, %d busy\n", db_pool_num_idle(pool),
           db_pool_num_busy(pool));
    assert(db != NULL);
    db_sql(db, "select id from people limit 1");
    assertEqual(db_exec(db), 0);
    db_close(db);
  }

  return NULL;
}

static void testConcurrency() {
  pthread_t thread1;
  pthread_t thread2;
  pthread_t thread3;
  DBPool *pool = db_pool_create("postgresql://assis:assis@127.0.0.1:5432/assis",
                                false, 1, 4);

  assertEqual(db_pool_num_busy(pool), 0);
  assertEqual(db_pool_num_idle(pool), 1);

  pthread_create(&thread1, NULL, testConcurrencyWorker, pool);
  pthread_create(&thread2, NULL, testConcurrencyWorker, pool);
  pthread_create(&thread3, NULL, testConcurrencyWorker, pool);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  pthread_join(thread3, NULL);

  assertEqual(db_pool_num_busy(pool), 0);
  assertEqual(db_pool_num_idle(pool), 1);

  db_pool_destroy(pool);
}

int main() {
  // log_ignore("db", LOG_DBUG);

  testOpen();

  testPoolCreateGetAndClose();

  testConcurrency();

  return 0;
}
