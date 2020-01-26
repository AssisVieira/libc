#include "db.h"
#include "log/log.h"
#include <postgresql/libpq-fe.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static void db_onConnect(void *instancia, int fd, IOEventType event);
static void db_onQueryRead(void *context, int fd, IOEventType event);

typedef struct DB {
  void *context;
  PGconn *conn;
  onDBConnected onConnected;
  onDBError onError;
  int fd;
  bool connected;
} DB;

typedef struct DBQuery {
  PGresult *result;
  onDBQueryResult onResult;
  void *context;
  bool done;
  bool error;
} DBQuery;

////////////////////////////////////////////////////////////////////////////////

static DB db = {
    .context = NULL,
    .conn = NULL,
    .onConnected = NULL,
    .onError = NULL,
    .fd = -1,
    .connected = false,
};

////////////////////////////////////////////////////////////////////////////////

int db_close() {
  if (db.conn != NULL) {
    PQfinish(db.conn);
    return 0;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

int db_query(const char *str, void *context, onDBQueryResult onQueryResult) {

  DBQuery *query = malloc(sizeof(DBQuery));
  query->onResult = onQueryResult;
  query->context = context;
  query->result = NULL;
  query->done = false;
  query->error = false;

  ioevent_listen(db.fd, IOEVENT_TYPE_READ, query, db_onQueryRead);
  ioevent_nolisten(db.fd, IOEVENT_TYPE_WRITE);

  if (!PQsendQuery(db.conn, str)) {
    log_erro("db", "%s", PQerrorMessage(db.conn));
    query->error = true;
    query->onResult(query, db.context);
    free(query);
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool db_queryIsDone(const DBQuery *query) { return query->done; }

////////////////////////////////////////////////////////////////////////////////

bool db_queryIsError(const DBQuery *query) { return query->error; }

////////////////////////////////////////////////////////////////////////////////

int db_queryCount(DBQuery *query) { return PQntuples(query->result); }

////////////////////////////////////////////////////////////////////////////////

const char *db_value(DBQuery *query, int nReg, int nCol) {
  return PQgetvalue(query->result, nReg, nCol);
}

////////////////////////////////////////////////////////////////////////////////

static void db_onQueryRead(void *context, int fd, IOEventType event) {
  DBQuery *query = context;
  (void)fd;
  (void)event;

  do {
    int r = PQconsumeInput(db.conn);

    log_dbug("db", "PQconsumeInput() = %d\n", r);

    if (!r) {
      log_erro("db", "%s\n", PQerrorMessage(db.conn));
      // Se houve problema com PQconsumeInput(), mas ainda está ocupado
      // (PQisBusy), então acreditamos que o problema é passageiro e que os
      // dados virão.
      if (!PQisBusy(db.conn)) {
        query->done = true;
        query->error = true;
        query->onResult(query, query->context);
        free(query);
      }
      return;
    }

    if (PQisBusy(db.conn)) {
      log_dbug("db", "PQisBusy()\n");
      return;
    }

    query->result = PQgetResult(db.conn);

    if (query->result != NULL) {
      if (PQresultStatus(query->result) != PGRES_TUPLES_OK) {
        log_erro("db", "Status = %s; Message: %s\n",
                 PQresStatus(PQresultStatus(query->result)),
                 PQresultErrorMessage(query->result));
        query->error = true;
      }
    }

    query->onResult(query, query->context);

    PQclear(query->result);

  } while (query->result != NULL);

  query->done = true;

  query->onResult(query, query->context);

  free(query);
}

////////////////////////////////////////////////////////////////////////////////

static void db_onConnect(void *context, int fd, IOEventType event) {
  (void)context;
  (void)fd;
  (void)event;

  int polling = PQconnectPoll(db.conn);

  if (polling == PGRES_POLLING_READING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_READING\n");
    ioevent_listen(db.fd, IOEVENT_TYPE_READ, NULL, db_onConnect);
    ioevent_nolisten(db.fd, IOEVENT_TYPE_WRITE);
    return;
  }

  if (polling == PGRES_POLLING_WRITING) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_WRITING\n");
    ioevent_nolisten(db.fd, IOEVENT_TYPE_READ);
    ioevent_listen(db.fd, IOEVENT_TYPE_WRITE, NULL, db_onConnect);
    return;
  }

  if (polling == PGRES_POLLING_OK) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_OK\n");
    if (PQsetnonblocking(db.conn, 1 /*NON-BLOCKING*/) == 0) {
      db.connected = true;
      db.onConnected(db.context);
    } else {
      db.onError(db.context);
    }
    return;
  }

  if (polling == PGRES_POLLING_FAILED) {
    log_dbug("db", "PQconnectPoll() : PGRES_POLLING_FAILED\n");
    log_erro("db", "%s", PQerrorMessage(db.conn));
    db.onError(db.context);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

int db_open(const char *strCon, void *context, onDBConnected onDBConnected,
            onDBError onDBError) {

  db.context = context;
  db.onConnected = onDBConnected;
  db.onError = onDBError;
  db.conn = NULL;
  db.connected = false;
  db.fd = -1;

  db.conn = PQconnectStart(strCon);

  if (db.conn == NULL) {
    log_erro("db", "PQconnectStart()\n");
    db_close();
    return -1;
  }

  if (PQstatus(db.conn) == CONNECTION_BAD) {
    log_erro("db", "PQstatus()\n");
    db_close();
    return -1;
  }

  if (PQsetnonblocking(db.conn, 1) != 0) {
    log_erro("db", "PQsetnonblocking()\n");
    db_close();
    return -1;
  }

  db.fd = PQsocket(db.conn);

  if (db.fd < 0) {
    log_erro("db", "PQsocket(); fd = %d\n", db.fd);
    db_close();
    return -1;
  }

  if (ioevent_install(db.fd)) {
    log_erro("db", "ioevent_install(); fd = %d\n", db.fd);
    db_close();
    return -1;
  }

  if (ioevent_listen(db.fd, IOEVENT_TYPE_WRITE, NULL, db_onConnect)) {
    log_erro("db", "ioevent_listen(); fd = %d\n", db.fd);
    db_close();
    return -1;
  }

  return 0;
}
