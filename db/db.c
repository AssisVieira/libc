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

////////////////////////////////////////////////////////////////////////////////

static void db_onEventConnect(void *instancia, int fd, IOEvent event);
static void db_onEventQuery(void *context, int fd, IOEvent event);
static void db_destroy(DB *db);

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
  bool idle;
  bool error;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct DBPool {
  DB *dbs;
  size_t min;
  size_t max;
  bool debug;
} DBPool;

////////////////////////////////////////////////////////////////////////////////

static DBPool pool = {
    .dbs = NULL,
    .min = 0,
    .max = 0,
    .debug = true,
};

////////////////////////////////////////////////////////////////////////////////

int db_openPool(int min, int max) {
  pool.min = min;
  pool.max = max;

  char *strConn = getenv("DB_STRING_CONNECTION");

  if (strConn == NULL || strlen(strConn) == 0) {
    log_erro("db",
             "Variável de ambiente não definida: DB_STRING_CONNECTION. "
             "Exemplo: export DB_STRING_CONNECTION=\"host=127.0.0.1 port=5433 "
             "user=assis password=assis dbname=assis sslmode=require\"\n");
    return -1;
  }

  log_info("db", "Inicializando pool. Minímo ociosas: %d. Máximo ativas: %d\n",
           min, max);

  pool.dbs = malloc(sizeof(DB) * max);

  for (int i = 0; i < max; i++) {
    pool.dbs[i].context = NULL;
    pool.dbs[i].onCmdResult = NULL;
    pool.dbs[i].paramsLen = 0;
    pool.dbs[i].result = NULL;
    pool.dbs[i].sql = NULL;
    pool.dbs[i].idle = true;
    pool.dbs[i].conn = NULL;
    pool.dbs[i].error = false;
  }

  for (int i = 0; i < min; i++) {
    DB *db = &pool.dbs[i];

    db->conn = PQconnectStart(strConn);

    if (PQstatus(db->conn) == CONNECTION_BAD) {
      log_erro("db", "Erro ao abrir conexão no slot %d: PQstatus().\n", i);
      db_destroy(db);
      return -1;
    }

    if (PQsetnonblocking(db->conn, 1) != 0) {
      log_erro("db", "Erro ao abrir conexão no slot %d: PQsetnonblocking().\n",
               i);
      db_destroy(db);
      return -1;
    }

    if (PQsocket(db->conn) < 0) {
      log_erro("db", "Erro ao abrir conexão no slot %d: PQsocket().\n", i);
      db_destroy(db);
      return -1;
    }

    if (io_add(PQsocket(db->conn), IO_WRITE, db, db_onEventConnect)) {
      log_erro("db",
               "Erro ao abrir conexão no slot %d: "
               "io_listen(IO_ERROR).\n",
               i);
      db_destroy(db);
      return -1;
    }
  }

  return 0;
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
      continue;
    }

    db = &pool.dbs[i];

    nIdle++;
    // break;
  }

  log_info("db", "Obtendo conexão, restando: %d de %d.\n", nIdle - 1, pool.max);

  if (db != NULL) {
    db->idle = false;
    return db;
  }

  log_erro("db", "Sem conexão disponível.\n");

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

void db_close(DB *db) {
  if (db == NULL) return;

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

  log_dbug("db", "send: %s\n", db->sql);

  if (!PQsendQueryParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                         NULL, NULL, PG_FORMAT_TEXT)) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->error = true;
    db->onCmdResult(db);
  }
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

////////////////////////////////////////////////////////////////////////////////

static void db_onEventConnect(void *context, int fd, IOEvent event) {
  DB *db = context;
  (void)fd;

  if (event & IO_ERROR) {
    // DB *db = context;
    log_erro("db", "Erro ao abrir conexão: db_onSocketError(fd = %d).\n", fd);
    db_destroy(db);
    return;
  }

  int polling = PQconnectPoll(db->conn);

  if (polling == PGRES_POLLING_READING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_READING\n");

    if (io_mod(PQsocket(db->conn), IO_READ, db, db_onEventConnect)) {
      log_erro("db",
               "Erro ao abrir conexão: db_onConnect(fd = %d, pooling = %d).\n",
               fd, polling);
      db_destroy(db);
      return;
    }

    return;
  }

  if (polling == PGRES_POLLING_WRITING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_WRITING\n");

    if (io_mod(PQsocket(db->conn), IO_WRITE, db, db_onEventConnect)) {
      log_erro("db",
               "Erro ao abrir conexão: db_onConnect(fd = %d, pooling = %d).\n",
               fd, polling);
      db_destroy(db);
      return;
    }

    return;
  }

  if (polling == PGRES_POLLING_OK) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_OK\n");
    log_dbug("db", "PQstatus() : %d\n", PQstatus(db->conn));

    if (io_mod(PQsocket(db->conn), IO_READ, db, db_onEventQuery)) {
      log_erro("db",
               "Erro ao abrir conexão: db_onConnect(fd = %d, pooling = "
               "PGRES_POLLING_OK).\n",
               fd);
      db_destroy(db);
      return;
    }

    if (PQsetnonblocking(db->conn, 1 /*NON-BLOCKING*/) == 0) {
      log_info("db", "Conexão estabelecida (fd = %d).\n", fd);
    } else {
      log_erro("db",
               "Erro ao abrir conexão: db_onConnect(fd = %d, pooling = "
               "PGRES_POLLING_OK) -> "
               "PQsetnonblocking().\n",
               fd);
      db_destroy(db);
    }
    return;
  }

  if (polling == PGRES_POLLING_FAILED) {
    log_erro("db",
             "Erro ao abrir conexão: db_onConnect(fd = %d, pooling = "
             "PGRES_POLLING_FAILED): %s",
             fd, PQerrorMessage(db->conn));
    db_destroy(db);
    return;
  }
}
