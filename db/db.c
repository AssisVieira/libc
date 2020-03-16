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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "queue/queue.h"

////////////////////////////////////////////////////////////////////////////////

static void db_onEventQuery(void *context, int fd, IOEvent event);
static void db_destroy(DB *db);
static int db_begin(DB *db);
static int db_commit2(DB *db, bool chained);

////////////////////////////////////////////////////////////////////////////////

#define PG_FORMAT_TEXT 0
#define PG_FORMAT_BIN 1

////////////////////////////////////////////////////////////////////////////////

struct DB {
  PGconn *conn;
  onDBCallback onCmdResult;
  void *context;
  const char *sql;
  const char *params[DB_SQL_PARAMS_MAX];
  size_t paramsLen;
  PGresult *result;
  bool error;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct DBPool {
  Queue *dbs;
  int min;
  int max;
  char *strConn;
  bool async;
} DBPool;

////////////////////////////////////////////////////////////////////////////////

DBPool *db_pool_create(const char *strConn, bool async, int min, int max) {
  DBPool *pool = malloc(sizeof(DBPool));
  pool->dbs = queue_create(max);
  pool->min = min;
  pool->max = max;
  pool->async = async;
  pool->strConn = strdup(strConn);

  for (int i = 0; i < min; i++) {
    DB *db = db_open(strConn, async);
    if (db != NULL) {
      queue_add(pool->dbs, db);
    }
  }

  log_info("db", "Pool created: %d min, %d max.\n", min, max);

  return pool;
}

////////////////////////////////////////////////////////////////////////////////

DBPool *db_pool_destrou(DBPool *pool) {
  queue_destroy(pool->)

  return pool;
}

////////////////////////////////////////////////////////////////////////////////

DB * db_open(const char *strConn, bool async) {
  if (strConn == NULL) {
    strConn = getenv("DB_STRING_CONNECTION");
    if (strConn == NULL || strlen(strConn) == 0) {
      log_erro("db",
              "Variável de ambiente não definida: DB_STRING_CONNECTION. "
              "Exemplo: export DB_STRING_CONNECTION=\"host=xxx port=xxx "
              "user=xxx password=xxx dbname=xxx sslmode=require\"\n");
      return -1;
    }
  }

  DB *db = malloc(sizeof(DB));
  db->context = NULL;
  db->onCmdResult = NULL;
  db->paramsLen = 0;
  db->result = NULL;
  db->sql = NULL;
  db->conn = NULL;
  db->error = false;
  db->conn = PQconnectdb(strConn);

  if (PQstatus(db->conn) != CONNECTION_OK) {
    log_erro("db", "PQconnectdb() - %s\n", PQerrorMessage(db->conn));
    db_destroy(db);
    return NULL;
  }

  if (async) {
    if (PQsetnonblocking(db->conn, 1) != 0) {
      log_erro("db", "PQsetnonblocking().\n");
      db_destroy(db);
      return NULL;
    }

    if (io_add(io_current(), PQsocket(db->conn), IO_READ, db, db_onEventQuery)) {
      log_erro("db", "io_add().\n");
      db_destroy(db);
      return NULL;
    }
  }

  return db;
}

////////////////////////////////////////////////////////////////////////////////

void db_closePool() {
  for (int i = 0; i < pool.max; i++) {
    if (pool.dbs[i].conn != NULL) {
      db_destroy(&pool.dbs[i]);
    }
  }

  free(pool.dbs);
}

////////////////////////////////////////////////////////////////////////////////

DB *db_open() {
  DB *db = NULL;
  int nIdle = 0;

  for (int i = 0; i < pool.max; i++) {
    if (!pool.dbs[i].idle) {
      continue;
    }

    if (PQstatus(pool.dbs[i].conn) != CONNECTION_OK) {
      log_info("db", "connection %d: %d\n", i, PQstatus(pool.dbs[i].conn));
      continue;
    }

    db = &pool.dbs[i];

    nIdle++;
    // break;
  }

  log_info("db", "Obtendo conexão, restando: %d de %d.\n", nIdle - 1, pool.max);

  if (db != NULL) {
    db->idle = false;
    
    if (db_begin(db)) {
      return NULL;
    }

    return db;
  }

  log_warn("db", "Sem conexão disponível.\n");

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

void db_sql(DB *db, const char *sql) {
  if (db == NULL) return;
  db_clear(db);
  db->sql = sql;
}

////////////////////////////////////////////////////////////////////////////////

void db_param(DB *db, const char *value) {
  if (db == NULL) return;
  size_t size = strlen(value) + 1;
  char *buff = malloc(size);
  memcpy(buff, value, size);
  db->params[db->paramsLen++] = buff;
}

////////////////////////////////////////////////////////////////////////////////

void db_paramInt(DB *db, int value) {
  if (db == NULL) return;
  size_t size = 32;
  char *buff = malloc(size);
  snprintf(buff, size, "%d", value);
  db->params[db->paramsLen++] = buff;
}

////////////////////////////////////////////////////////////////////////////////

void *db_context(DB *db) {
  if (db == NULL) return NULL;
  return db->context;
}

void db_clear(DB *db) {
  if (db == NULL) return;
  while (db->result != NULL) {
    PQclear(db->result);
    db->result = PQgetResult(db->conn);
  }
  db->sql = NULL;

  for (int i = 0; i < db->paramsLen; i++) {
    free((void *)db->params[i]);
  }

  db->paramsLen = 0;

  db->onCmdResult = NULL;
}

////////////////////////////////////////////////////////////////////////////////

int db_rollback(DB *db) {
  db_sql(db, "ROLLBACK");
  if (db_exec(db)) {
    log_erro("db", "rollback fail.\n");
    db_destroy(db);
    return -1;
  }
  if (db_begin(db)) {
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int db_commit(DB *db) {
  return db_commit2(db, true);
}

////////////////////////////////////////////////////////////////////////////////

static int db_commit2(DB *db, bool chained) {
  db_sql(db, "COMMIT");
  if (db_exec(db)) {
    log_erro("db", "commit fail.\n");
    db_destroy(db);
    return -1;
  }
  if (chained && db_begin(db)) {
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int db_begin(DB *db) {
  db_sql(db, "BEGIN");
  if (db_exec(db)) {
    log_erro("db", "begin fail.\n");
    db_destroy(db);
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void db_close(DB *db) {
  if (db == NULL) return;

  db_commit2(db, false);

  db_clear(db);

  db->context = NULL;
  db->onCmdResult = NULL;
  db->paramsLen = 0;
  db->sql = NULL;
  db->idle = true;
  db->error = false;

  if (pool.debug) {
    int nIdle = 0;
    for (int i = 0; i < pool.max; i++) {
      if (!pool.dbs[i].idle) {
        continue;
      }

      if (PQstatus(pool.dbs[i].conn) != CONNECTION_OK) {
        continue;
      }

      nIdle++;
      // break;
    }
    log_dbug("db", "Devolvendo conexão, restando: %d de %d.\n", nIdle,
             pool.max);
  } else {
    log_dbug("db", "Devolvendo conexão.\n");
  }
}

////////////////////////////////////////////////////////////////////////////////

static void db_destroy(DB *db) {
  if (db == NULL) return;

  db_clear(db);
  PQfinish(db->conn);
  db->conn = NULL;
  db->context = NULL;
  db->onCmdResult = NULL;
  db->paramsLen = 0;
  db->sql = NULL;
  db->idle = true;
  db->error = false;
}

////////////////////////////////////////////////////////////////////////////////

void db_send(DB *db, void *context, onDBCallback onCmdResult) {
  if (db == NULL) return;

  db->onCmdResult = onCmdResult;
  db->context = context;

  log_dbug("db", "%s;\n", db->sql);

  if (!PQsendQueryParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                         NULL, NULL, PG_FORMAT_TEXT)) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->error = true;
    db->onCmdResult(db);
  }
}

////////////////////////////////////////////////////////////////////////////////

int db_exec(DB *db) {
  if (db == NULL) return -1;

  db->onCmdResult = NULL;
  db->context = NULL;

  log_dbug("db", "%s;\n", db->sql);

  db->result = PQexecParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                         NULL, NULL, PG_FORMAT_TEXT);

  if (PQresultStatus(db->result) != PGRES_TUPLES_OK && PQresultStatus(db->result) != PGRES_COMMAND_OK) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->error = true;
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int db_count(DB *db) {
  if (db == NULL) return -1;
  return PQntuples(db->result);
}

////////////////////////////////////////////////////////////////////////////////

const char *db_value(DB *db, int row, int col) {
  if (db == NULL) return NULL;
  return PQgetvalue(db->result, row, col);
}

////////////////////////////////////////////////////////////////////////////////

static void db_onEventQuery(void *context, int fd, IOEvent event) {
  DB *db = context;
  (void)fd;
  (void)event;

  if (!PQconsumeInput(db->conn)) {
    log_erro("db", "%s\n", PQerrorMessage(db->conn));
    // Se houve problema com PQconsumeInput(), mas ainda está ocupado
    // (PQisBusy), então acreditamos que o problema é passageiro e que os
    // dados virão.
    if (!PQisBusy(db->conn)) {
      db->error = true;
      db->onCmdResult(db);
    }
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
      db->onCmdResult(db);
    }
    PQclear(db->result);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool db_error(DB *db) { return (db == NULL || db->error); }

