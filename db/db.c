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

#include <postgresql/libpq-fe.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "log/log.h"
#include "queue/queue.h"

////////////////////////////////////////////////////////////////////////////////
/// DEFINES ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define PG_FORMAT_TEXT 0
#define PG_FORMAT_BIN 1

////////////////////////////////////////////////////////////////////////////////
/// PROTOTYPES /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void db_onEventQuery(void *context, int fd, IOEvent event);
static int db_begin(DB *db);
static int db_commit_intern(DB *db, bool chained);
static DB *db_open_intern(const char *strConn, bool async, DBPool *pool);
static int db_connectOrReset(DB *db, const char *strConn);

////////////////////////////////////////////////////////////////////////////////
/// TYPES //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct DB {
  PGconn *conn;
  onDBCallback onCmdResult;
  void *context;
  const char *sql;
  const char *params[DB_SQL_PARAMS_MAX];
  size_t paramsLen;
  PGresult *result;
  DBPool *pool;

  // Esta flag deve ser marcada sempre que um erro ocorrer em qualquer operação
  // com a conexão, assíncrono ou não. Conexões marcadas com esta flag não
  // podem ser recuperadas e sempre serão destruídas quando forem encerradas com
  // db_close(). Com exceção da db_close(), qualquer outra função que utilize
  // uma conexão, deve antes consultar esta flag através da função db_error().
  // Caso a flag esteja marcada, a função deverá ser cancelada.
  bool error;

  // Flag para conexões assíncronas. Se marcada, todas as operações da conexão
  // devem ser assíncronas. A conexão assíncrona que tentar realizar operação
  // síncrona ou vice e versa, deverá ser marcada com erro.
  bool async;

  // Flag que deve ser habilitada quando a conexão estiver dentro de um
  // bloco de transação não concluído. Isto é, um conexão iniciada com BEGIN
  // mas sem COMMIT e nem ROLLBACK.
  bool transaction;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct DBPool {
  // Fila de conexões ociosas.
  Queue *dbs;

  // Número máximo de conexões ociosas.
  size_t maxIdle;

  // Número máximo de conexões ocupadas.
  size_t maxBusy;

  // Número de conexões ocupadas.
  atomic_size_t busy;

  // String de conexão utilizada em todas as conexões.
  char *strConn;

  // Habilita ou não conexões assíncronas. Após a criação do pool, esta flag
  // não deve ser modificada.
  bool async;

  // Flag de ativação do pool. Se desativado, o pool é esvaziado ao liberar as
  // conexões ociosas com db_pool_get() e destruir as conexões devolvidas com
  // db_close(). Esta flag pode ser modificada durante o ciclo de vida do pool.
  // O pool deve ser desativado sempre que houver erro em qualquer operação
  // envolvendo o pool. Ao fazer isso, db_close() irá destruir as conexões, em
  // vez devolver as conexões pro pool defeituoso.
  atomic_bool enabled;
} DBPool;

////////////////////////////////////////////////////////////////////////////////
/// DB POOL ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DBPool *db_pool_create(const char *strConn, bool async, size_t maxIdle,
                       size_t maxBusy) {
  DBPool *pool = malloc(sizeof(DBPool));
  pool->dbs = queue_create(maxIdle);
  pool->maxIdle = maxIdle;
  pool->maxBusy = maxBusy;
  pool->busy = 0;
  pool->async = async;
  pool->enabled = true;
  pool->strConn = strdup(strConn);

  for (size_t i = 0; i < maxIdle; i++) {
    DB *db = db_open_intern(strConn, async, pool);

    if (db == NULL) {
      db_pool_destroy(pool);
      return NULL;
    }

    if (!queue_add(pool->dbs, db, false)) {
      db->error = true;
      db_close(db);
      db_pool_destroy(pool);
      return NULL;
    }
  }

  log_info("db", "db_pool_create(): %d idle, %d busy.\n",
           queue_count(pool->dbs), pool->busy);

  return pool;
}

////////////////////////////////////////////////////////////////////////////////

void db_pool_enable(DBPool *pool) { pool->enabled = true; }

////////////////////////////////////////////////////////////////////////////////

void db_pool_disable(DBPool *pool) { pool->enabled = false; }

////////////////////////////////////////////////////////////////////////////////

DB *db_pool_get(DBPool *pool) {
  DB *db = queue_get(pool->dbs, false);

  if (db != NULL) {
    pool->busy++;
  } else {
    while (true) {
      size_t oldBusy = pool->busy;

      if (oldBusy >= pool->maxBusy) break;

      if (atomic_compare_exchange_weak(&pool->busy, &oldBusy, oldBusy + 1)) {
        db = db_open_intern(pool->strConn, pool->async, pool);
        break;
      }
    }
  }

  // Se não houver conexão ociosa ou o número de conexões ocupadas chegou ao
  // limite máximo ou a tentativa de criar uma nova conexão não teve sucesso,
  // então aguarda alguma conexão ocupada retornar ao pool.
  if (db == NULL && pool->busy > 0) {
    db = queue_get(pool->dbs, true);
  }

  // Se houver algum erro no início da transação, reinicia a conexão e tenta
  // novamente iniciar a transação, se falhar novamente, então cancela a
  // operação.
  if (db_begin(db)) {
    if (db_connectOrReset(db, NULL)) {
      db->error = true;
      db_close(db);
      return NULL;
    }

    if (db_begin(db)) {
      db->error = true;
      db_close(db);
      return NULL;
    }
  }

  return db;
}

////////////////////////////////////////////////////////////////////////////////

void db_pool_destroy(DBPool *pool) {
  DB *db = NULL;

  if (pool == NULL) return;

  // Desabilita o pool antes de fechar as conexões com db_close(), caso
  // contrário, as conexões voltarão para o pool.
  db_pool_disable(pool);

  while ((db = queue_get(pool->dbs, false)) != NULL) {
    db_close(db);
  }

  queue_destroy(pool->dbs);
  free(pool->strConn);
  free(pool);
}

////////////////////////////////////////////////////////////////////////////////
/// DB /////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DB *db_open(const char *strConn, bool async) {
  if (strConn == NULL) return NULL;

  DB *db = db_open_intern(strConn, async, NULL);

  if (db == NULL || db_begin(db)) {
    return NULL;
  }

  return db;
}

////////////////////////////////////////////////////////////////////////////////

static int db_connectOrReset(DB *db, const char *strConn) {
  if (db == NULL) return -1;

  if (strConn != NULL) {
    if (db->conn != NULL) return -1;
    db->conn = PQconnectdb(strConn);
  } else {
    if (db->conn == NULL) return -1;
    PQreset(db->conn);
  }

  if (PQstatus(db->conn) != CONNECTION_OK) {
    log_erro("db", "PQconnectdb() - %s\n", PQerrorMessage(db->conn));
    return -1;
  }

  if (db->async) {
    if (PQsetnonblocking(db->conn, 1) != 0) {
      log_erro("db", "PQsetnonblocking().\n");
      return -1;
    }

    if (io_add(io_current(), PQsocket(db->conn), IO_READ, db,
               db_onEventQuery)) {
      log_erro("db", "io_add().\n");
      return -1;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static DB *db_open_intern(const char *strConn, bool async, DBPool *pool) {
  if (strConn == NULL) return NULL;

  DB *db = malloc(sizeof(DB));
  db->onCmdResult = NULL;
  db->paramsLen = 0;
  db->result = NULL;
  db->sql = NULL;
  db->conn = NULL;
  db->error = false;
  db->conn = NULL;
  db->pool = pool;
  db->async = async;
  db->transaction = false;

  if (db_connectOrReset(db, strConn)) {
    db->error = true;
    db_close(db);
    return NULL;
  }

  return db;
}

////////////////////////////////////////////////////////////////////////////////

void db_sql(DB *db, const char *sql) {
  if (db_error(db)) return;
  db_clear(db);
  db->sql = sql;
}

////////////////////////////////////////////////////////////////////////////////

void db_param(DB *db, const char *value) {
  if (db_error(db)) return;
  db->params[db->paramsLen++] = strdup(value);
}

////////////////////////////////////////////////////////////////////////////////

void db_paramInt(DB *db, int value) {
  if (db_error(db)) return;
  size_t size = 32;
  char *buff = malloc(size);
  snprintf(buff, size, "%d", value);
  db->params[db->paramsLen++] = buff;
}

////////////////////////////////////////////////////////////////////////////////

void db_paramDouble(DB *db, double value) {
  if (db_error(db)) return;
  size_t size = 62;
  char *buff = malloc(size);
  snprintf(buff, size, "%lf", value);
  db->params[db->paramsLen++] = buff;
}

////////////////////////////////////////////////////////////////////////////////

void db_clear(DB *db) {
  if (db_error(db)) return;

  while (db->result != NULL) {
    PQclear(db->result);
    db->result = PQgetResult(db->conn);
  }

  for (int i = 0; i < db->paramsLen; i++) {
    free((void *)db->params[i]);
  }

  db->sql = NULL;
  db->paramsLen = 0;
  db->onCmdResult = NULL;
  db->context = NULL;
}

////////////////////////////////////////////////////////////////////////////////

int db_rollback(DB *db) {
  if (db_error(db)) return -1;

  db_sql(db, "ROLLBACK");

  if (db_exec(db)) {
    log_erro("db", "rollback fail.\n");
    db->error = true;
    return -1;
  }

  db->transaction = false;

  if (db_begin(db)) {
    db->error = true;
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int db_commit(DB *db) { return db_commit_intern(db, true); }

////////////////////////////////////////////////////////////////////////////////

static int db_commit_intern(DB *db, bool chained) {
  if (db_error(db)) return -1;

  db_sql(db, "COMMIT");

  if (db_exec(db)) {
    log_erro("db", "commit fail.\n");
    db->error = true;
    return -1;
  }

  db->transaction = false;

  if (chained && db_begin(db)) {
    db->error = true;
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int db_begin(DB *db) {
  if (db_error(db)) return -1;

  db_sql(db, "BEGIN");

  if (db_exec(db)) {
    log_erro("db", "begin fail.\n");
    db->error = true;
    return -1;
  }

  db->transaction = true;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// TODO retornar 0, se não houve nenhum erro durante toda a operação com a conexão,
//               1, caso contrário.
//               Em casos em que a operação é apenas um select, se a chamada para 
//               db_close() retornar -1, logo toda a consulta deve ser descartada.
//               Erros em consulta podem acontecer na conversão de tipos, 
//               consequentemente na leitura equivocada de algum dado.
void db_close(DB *db) {
  if (db == NULL) return;

  if (!db->error && db->transaction) db_commit_intern(db, false);

  db_clear(db);

  if (db->pool != NULL) {
    // Se a conexão veio do pool, verifica se destrói ou volta pro pool...
    db->pool->busy--;

    if (db->error || !db->pool->enabled ||
        !queue_add(db->pool->dbs, db, false)) {
      goto destroy;
    }

    return;
  }

destroy:
  fprintf(stdout, "Connection destroyed -> %p / %p\n", db, db->conn);
  fflush(stdout);
  PQfinish(db->conn);
  free(db);
}

////////////////////////////////////////////////////////////////////////////////

void db_send(DB *db, void *context, onDBCallback onCmdResult) {
  if (db == NULL) {
    onCmdResult(db, context, true);
    return;
  }

  db->onCmdResult = onCmdResult;
  db->context = context;

  if (db_error(db)) {
    db->onCmdResult(db, context, true);
    return;
  }

  if (!db->async) {
    db->onCmdResult(db, context, true);
    return;
  }

  log_dbug("db", "%s;\n", db->sql);

  if (!PQsendQueryParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                         NULL, NULL, PG_FORMAT_TEXT)) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->error = true;
    db->onCmdResult(db, context, true);
  }
}

////////////////////////////////////////////////////////////////////////////////

int db_exec(DB *db) {
  if (db_error(db)) return -1;

  if (db->async) return -1;

  db->onCmdResult = NULL;

  log_dbug("db", "%s;\n", db->sql);

  db->result = PQexecParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                            NULL, NULL, PG_FORMAT_TEXT);

  if (PQresultStatus(db->result) != PGRES_TUPLES_OK &&
      PQresultStatus(db->result) != PGRES_COMMAND_OK) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->error = true;
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

size_t db_count(DB *db) {
  if (db == NULL) return -1;
  return PQntuples(db->result);
}

////////////////////////////////////////////////////////////////////////////////

const char *db_value(DB *db, int row, int col) {
  if (db == NULL) return NULL;
  return PQgetvalue(db->result, row, col);
}

double db_valueDouble(DB *db, int nReg, int nCol) {
  double x;
  const char *value = db_value(db, nReg, nCol);
  char *p = NULL;
  x = strtod(value, &p);
  if ((x == 0 || x == HUGE_VALF || x == HUGE_VALL) && errno == ERANGE) {
    fprintf(stderr, "%s(): erange.\n", __FUNCTION__);
    db->error = true;
    return 0;
  }
  return x;
}

int db_valueInt(DB *db, int nReg, int nCol) {
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void db_onEventQuery(void *context, int fd, IOEvent event) {
  DB *db = context;
  (void)fd;
  (void)event;

  if (!PQconsumeInput(db->conn)) {
    log_erro("db", "%s\n", PQerrorMessage(db->conn));
    db->error = true;
    db->onCmdResult(db, db->context, db->error);
    return;
  }

  if (PQisBusy(db->conn)) {
    log_dbug("db", "PQisBusy()\n");
    return;
  }

  while ((db->result = PQgetResult(db->conn)) != NULL) {
    if (!db->error) {
      if (PQresultStatus(db->result) != PGRES_TUPLES_OK &&
          PQresultStatus(db->result) != PGRES_COMMAND_OK) {
        log_erro("db", "Status = %s; Message: %s\n",
                 PQresStatus(PQresultStatus(db->result)),
                 PQresultErrorMessage(db->result));
        db->error = true;
      }
      db->onCmdResult(db, db->context, db->error);
    }
    PQclear(db->result);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool db_error(DB *db) { return (db == NULL || db->error); }

////////////////////////////////////////////////////////////////////////////////

int db_pool_num_busy(const DBPool *pool) { return pool->busy; }

////////////////////////////////////////////////////////////////////////////////

int db_pool_num_idle(const DBPool *pool) { return queue_count(pool->dbs); }
