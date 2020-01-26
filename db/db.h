#ifndef DB_H
#define DB_H

#include "ioevent/ioevent.h"

typedef struct DB DB;
typedef struct DBQuery DBQuery;

typedef void (*onDBConnected)(void *context);
typedef void (*onDBError)(void *context);
typedef void (*onDBQueryResult)(DBQuery *query, void *context);

int db_open(const char *strCon, void *context, onDBConnected onDBConnected,
            onDBError onDBError);

int db_close();

int db_query(const char *str, void *context, onDBQueryResult onQueryResult);

void db_queryClose(DBQuery *query);

const char *db_value(DBQuery *query, int nReg, int nCol);

int db_queryCount(DBQuery *query);

bool db_queryIsDone(const DBQuery *query);
bool db_queryIsError(const DBQuery *query);

#endif
