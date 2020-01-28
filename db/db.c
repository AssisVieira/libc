#include "db.h"
#include "log/log.h"
#include <postgresql/libpq-fe.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void db_onConnect(void *instancia, int fd, IOEventType event);
static void db_onQueryResult(void *context, int fd, IOEventType event);
static void db_onSocketError(void *context, int fd, IOEventType event);

#define PG_FORMAT_TEXT 0
#define PG_FORMAT_BIN 1

typedef struct DB {
  PGconn *conn;
  onDBCallback onDBConnected;
  onDBCallback onDBError;
  onDBCallback onCmdResult;
  onDBCallback onCmdError;
  void *context;
  const char *sql;
  const char *params[DB_SQL_PARAMS_MAX];
  size_t paramsLen;
  PGresult *result;
} DB;

DB db = {
    .conn = NULL,
    .onDBConnected = NULL,
    .onDBError = NULL,
    .onCmdResult = NULL,
    .onCmdError = NULL,
    .context = NULL,
    .sql = NULL,
    .paramsLen = 0,
    .result = NULL,
};

void db_open(void *context, onDBCallback onConnected, onDBCallback onError) {
  db.onDBConnected = onConnected;
  db.onDBError = onError;
  db.context = context;

  char *strConn = getenv("DB_STRING_CONNECTION");

  if (strConn == NULL || strlen(strConn) == 0) {
    log_erro("db",
             "Variável de ambiente não definida: DB_STRING_CONNECTION. "
             "Exemplo: export DB_STRING_CONNECTION=\"host=127.0.0.1 port=5433 "
             "user=assis password=assis dbname=assis sslmode=require\"\n");
    onError(&db);
    return;
  }

  db.conn = PQconnectStart(strConn);

  if (db.conn == NULL) {
    log_erro("db", "PQconnectStart()\n");
    onError(&db);
    return;
  }

  if (PQstatus(db.conn) == CONNECTION_BAD) {
    log_erro("db", "PQstatus() = %d\n", PQstatus(db.conn));
    onError(&db);
    return;
  }

  if (PQsetnonblocking(db.conn, 1) != 0) {
    log_erro("db", "PQsetnonblocking()\n");
    onError(&db);
    return;
  }

  if (PQsocket(db.conn) < 0) {
    log_erro("db", "PQsocket(); fd = %d\n", PQsocket(db.conn));
    onError(&db);
    return;
  }

  if (ioevent_install(PQsocket(db.conn), false)) {
    log_erro("db", "ioevent_install(); fd = %d\n", PQsocket(db.conn));
    onError(&db);
    return;
  }

  if (ioevent_listen(PQsocket(db.conn), IOEVENT_TYPE_WRITE, &db,
                     db_onConnect)) {
    log_erro("db", "ioevent_listen(IOEVENT_TYPE_WRITE); fd = %d\n",
             PQsocket(db.conn));
    onError(&db);
    return;
  }

  if (ioevent_listen(PQsocket(db.conn), IOEVENT_TYPE_ERROR, &db,
                     db_onSocketError)) {
    log_erro("db", "ioevent_listen(IOEVENT_TYPE_ERROR); fd = %d\n",
             PQsocket(db.conn));
    onError(&db);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

void db_sql(DB *db, const char *sql) {
  db_clear(db);
  db->sql = sql;
}

////////////////////////////////////////////////////////////////////////////////

void db_param(DB *db, const char *value) {
  db->params[db->paramsLen++] = value;
}

////////////////////////////////////////////////////////////////////////////////

void *db_context(DB *db) { return db->context; }

void db_clear(DB *db) {
  while (db->result != NULL) {
    PQclear(db->result);
    db->result = PQgetResult(db->conn);
  }
  db->sql = NULL;
  db->paramsLen = 0;
  db->onCmdError = NULL;
  db->onCmdResult = NULL;
}

////////////////////////////////////////////////////////////////////////////////

void db_close(DB *db) {
  db_clear(db);
  PQfinish(db->conn);
  db->conn = NULL;
  db->context = NULL;
  db->onCmdError = NULL;
  db->onCmdResult = NULL;
  db->onDBConnected = NULL;
  db->onDBError = NULL;
  db->paramsLen = 0;
  db->sql = NULL;
}

////////////////////////////////////////////////////////////////////////////////

void db_send(DB *db, onDBCallback onCmdResult, onDBCallback onCmdError) {
  db->onCmdError = onCmdError;
  db->onCmdResult = onCmdResult;

  log_dbug("db", "send: %s\n", db->sql);

  if (!PQsendQueryParams(db->conn, db->sql, db->paramsLen, NULL, db->params,
                         NULL, NULL, PG_FORMAT_TEXT)) {
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->onCmdError(db);
  }
}

////////////////////////////////////////////////////////////////////////////////

int db_count(DB *db) { return PQntuples(db->result); }

////////////////////////////////////////////////////////////////////////////////

const char *db_value(DB *db, int row, int col) {
  return PQgetvalue(db->result, row, col);
}

////////////////////////////////////////////////////////////////////////////////

static void db_onQueryResult(void *context, int fd, IOEventType event) {
  DB *db = context;
  (void)fd;
  (void)event;
  bool hasError = false;

  if (!PQconsumeInput(db->conn)) {
    log_erro("db", "%s\n", PQerrorMessage(db->conn));
    // Se houve problema com PQconsumeInput(), mas ainda está ocupado
    // (PQisBusy), então acreditamos que o problema é passageiro e que os
    // dados virão.
    if (!PQisBusy(db->conn)) {
      db->onCmdError(db);
    }
    return;
  }

  if (PQisBusy(db->conn)) {
    log_dbug("db", "PQisBusy()\n");
    return;
  }

  while ((db->result = PQgetResult(db->conn)) != NULL) {
    if (!hasError) {
      if (PQresultStatus(db->result) != PGRES_TUPLES_OK &&
          PQresultStatus(db->result) != PGRES_COMMAND_OK) {
        log_erro("db", "Status = %s; Message: %s\n",
                 PQresStatus(PQresultStatus(db->result)),
                 PQresultErrorMessage(db->result));
        hasError = true;
        db->onCmdError(db);
      } else {
        db->onCmdResult(db);
      }
    }
    PQclear(db->result);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void db_onSocketError(void *context, int fd, IOEventType event) {
  DB *db = context;
  log_erro("db", "socket error.\n");
  db->onDBError(db);
}

////////////////////////////////////////////////////////////////////////////////

static void db_onConnect(void *context, int fd, IOEventType event) {
  DB *db = context;
  (void)fd;
  (void)event;

  int polling = PQconnectPoll(db->conn);

  if (polling == PGRES_POLLING_READING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_READING\n");

    if (ioevent_listen(PQsocket(db->conn), IOEVENT_TYPE_READ, db,
                       db_onConnect)) {
      db->onDBError(db);
      return;
    }

    if (ioevent_nolisten(PQsocket(db->conn), IOEVENT_TYPE_WRITE)) {
      db->onDBError(db);
      return;
    }

    return;
  }

  if (polling == PGRES_POLLING_WRITING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_WRITING\n");

    if (ioevent_listen(PQsocket(db->conn), IOEVENT_TYPE_WRITE, db,
                       db_onConnect)) {
      db->onDBError(db);
      return;
    }

    if (ioevent_nolisten(PQsocket(db->conn), IOEVENT_TYPE_READ)) {
      db->onDBError(db);
      return;
    }

    return;
  }

  if (polling == PGRES_POLLING_OK) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_OK\n");
    log_dbug("db", "PQstatus() : %d\n", PQstatus(db->conn));

    if (ioevent_nolisten(PQsocket(db->conn), IOEVENT_TYPE_READ)) {
      db->onDBError(db);
      return;
    }

    if (ioevent_nolisten(PQsocket(db->conn), IOEVENT_TYPE_WRITE)) {
      db->onDBError(db);
      return;
    }

    if (ioevent_listen(PQsocket(db->conn), IOEVENT_TYPE_READ, db,
                       db_onQueryResult)) {
      db->onDBError(db);
      return;
    }

    if (PQsetnonblocking(db->conn, 1 /*NON-BLOCKING*/) == 0) {
      db->onDBConnected(db);
    } else {
      log_erro("db", "PQsetnonblocking()\n");
      db->onDBError(db);
    }
    return;
  }

  if (polling == PGRES_POLLING_FAILED) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_FAILED\n");
    log_erro("db", "%s", PQerrorMessage(db->conn));
    db->onDBError(db);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////
