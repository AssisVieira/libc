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

#ifndef DB_H
#define DB_H

#include "ioevent/ioevent.h"

#define DB_SQL_PARAMS_MAX 32

typedef struct DB DB;

typedef void (*onDBCallback)(DB *db);

void db_open(void *context, onDBCallback onConnected, onDBCallback onError);

void db_close(DB *db);

void db_sql(DB *db, const char *sql);

void db_clear(DB *db);

void db_param(DB *db, const char *value);

void db_send(DB *db, onDBCallback onCmdResult, onDBCallback onCmdError);

const char *db_value(DB *db, int nReg, int nCol);

void *db_context(DB *db);

int db_count(DB *db);

#endif
