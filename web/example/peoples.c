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

#include "peoples.h"

#include <string.h>

#include "db/db.h"
#include "log/log.h"

////////////////////////////////////////////////////////////////////////////////

static void peoples_listOnResp(DB *db);

void peoples_list(PeoplesListSig *sig) {
  if (sig->page < 0) {
    sig->page = 0;
  }

  if (sig->pageSize < 10) {
    sig->pageSize = 10;
  }

  if (sig->pageSize > PEOPLES_LIST_MAX_PEOPLES) {
    sig->pageSize = PEOPLES_LIST_MAX_PEOPLES;
  }

  DB *db = db_open();

  db_sql(db,
         "select id, name, email from people where name ilike '%' || "
         "$1 || '%' OR email ilike '%' || $1 || '%' OFFSET $2 "
         "LIMIT $3");

  db_param(db, sig->query);
  db_paramInt(db, sig->page * sig->pageSize);
  db_paramInt(db, sig->pageSize);

  db_send(db, sig, peoples_listOnResp);
}

static void peoples_listOnResp(DB *db) {
  PeoplesListSig *sig = db_context(db);
  PeoplesStatus status = PEOPLES_ERROR;

  if (!db_error(db)) {
    status = PEOPLES_OK;
    sig->resp.peopleLen = db_count(db);
    for (int i = 0; i < db_count(db); i++) {
      strcpy(sig->resp.peoples[i].id, db_value(db, i, 0));
      strcpy(sig->resp.peoples[i].name, db_value(db, i, 1));
      strcpy(sig->resp.peoples[i].email, db_value(db, i, 2));
    }
  }

  db_close(db);
  return sig->callback(sig, status);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesAddResp(DB *db) {
  PeoplesAddSig *sig = db_context(db);
  PeoplesStatus status = PEOPLES_ERROR;

  if (!db_error(db)) {
    status = PEOPLES_OK;
  }

  db_close(db);
  return sig->callback(sig, status);
}

void peoples_add(PeoplesAddSig *sig) {
  if (sig->name == NULL || strlen(sig->name) == 0) {
    return sig->callback(sig, PEOPLES_NAME_EMPTY);
  }

  if (sig->email == NULL || strlen(sig->email) == 0) {
    return sig->callback(sig, PEOPLES_EMAIL_EMPTY);
  }

  DB *db = db_open();
  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, sig->name);
  db_param(db, sig->email);
  db_send(db, sig, onPeoplesAddResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesUpdateResp(DB *db) {
  PeoplesUpdateSig *sig = db_context(db);
  PeoplesStatus status = PEOPLES_ERROR;

  if (!db_error(db)) {
    status = PEOPLES_OK;
  }

  db_close(db);
  return sig->callback(sig, status);
}

void peoples_update(PeoplesUpdateSig *sig) {
  if (sig->id == NULL || strlen(sig->id) == 0) {
    return sig->callback(sig, PEOPLES_ID_EMPTY);
  }

  if (sig->name == NULL || strlen(sig->name) == 0) {
    return sig->callback(sig, PEOPLES_NAME_EMPTY);
  }

  if (sig->email == NULL || strlen(sig->email) == 0) {
    return sig->callback(sig, PEOPLES_EMAIL_EMPTY);
  }

  DB *db = db_open();

  db_sql(
      db,
      "update people name = $1::text, email = $2::text where id = $3::bigint");
  db_param(db, sig->name);
  db_param(db, sig->email);
  db_param(db, sig->id);
  db_send(db, sig, onPeoplesUpdateResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesRemoveResp(DB *db) {
  PeoplesRemoveSig *sig = db_context(db);
  PeoplesStatus status = PEOPLES_ERROR;

  if (!db_error(db)) {
    status = PEOPLES_OK;
  }

  db_close(db);
  return sig->callback(sig, status);
}

void peoples_remove(PeoplesRemoveSig *sig) {
  if (sig->id == NULL || strlen(sig->id) == 0) {
    return sig->callback(sig, PEOPLES_ID_EMPTY);
  }

  DB *db = db_open();

  db_sql(db, "delete people where id = $1::bigint");
  db_param(db, sig->id);
  db_send(db, sig, onPeoplesRemoveResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesDetailsResp(DB *db) {
  PeoplesDetailsSig *sig = db_context(db);
  PeoplesStatus status = PEOPLES_ERROR;

  if (!db_error(db) && db_count(db) == 1) {
    status = PEOPLES_OK;
    strcpy(sig->resp.id, db_value(db, 0, 0));
    strcpy(sig->resp.name, db_value(db, 0, 1));
    strcpy(sig->resp.email, db_value(db, 0, 2));
  }

  db_close(db);
  return sig->callback(sig, status);
}

void peoples_details(PeoplesDetailsSig *sig) {
  if (sig->id == NULL || strlen(sig->id) == 0) {
    return sig->callback(sig, PEOPLES_ID_EMPTY);
  }

  DB *db = db_open();

  db_sql(db, "select id, name, email from people where id = $1::bigint");
  db_param(db, sig->id);
  db_send(db, sig, onPeoplesDetailsResp);
}

////////////////////////////////////////////////////////////////////////////////
