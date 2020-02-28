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

static void onPeoplesListResp(DB *db) {
  PeoplesListSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  sig->resp.peopleLen = db_count(db);

  for (int i = 0; i < db_count(db); i++) {
    strcpy(sig->resp.peoples[i].id, db_value(db, i, 0));
    strcpy(sig->resp.peoples[i].name, db_value(db, i, 1));
    strcpy(sig->resp.peoples[i].email, db_value(db, i, 2));
  }

  return sig->callback(sig, PEOPLES_OK);
}

void peoples_list(PeoplesListSig *sig) {
  if (sig->page < 0) {
    sig->page = 0;
  }

  if (sig->pageSize < 10) {
    sig->pageSize = 10;
  }

  db_sql(sig->db,
         "select id, name, email from people where name ilike '%' || "
         "$1::text || '%' OR email ilike '%' || $1::text || '%'");

  db_param(sig->db, sig->query);

  db_send(sig->db, sig, onPeoplesListResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesAddResp(DB *db) {
  PeoplesAddSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  return sig->callback(sig, PEOPLES_OK);
}

void peoples_add(PeoplesAddSig *sig) {
  if (sig->name == NULL || strlen(sig->name) == 0) {
    return sig->callback(sig, PEOPLES_NAME_EMPTY);
  }

  if (sig->email == NULL || strlen(sig->email) == 0) {
    return sig->callback(sig, PEOPLES_EMAIL_EMPTY);
  }

  db_sql(sig->db,
         "insert into people (name, email) values ($1::text, $2::text)");
  db_param(sig->db, sig->name);
  db_param(sig->db, sig->email);
  db_send(sig->db, sig, onPeoplesAddResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesUpdateResp(DB *db) {
  PeoplesUpdateSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  return sig->callback(sig, PEOPLES_OK);
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

  db_sql(
      sig->db,
      "update people name = $1::text, email = $2::text where id = $3::bigint");
  db_param(sig->db, sig->name);
  db_param(sig->db, sig->email);
  db_param(sig->db, sig->id);
  db_send(sig->db, sig, onPeoplesUpdateResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesRemoveResp(DB *db) {
  PeoplesRemoveSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  return sig->callback(sig, PEOPLES_OK);
}

void peoples_remove(PeoplesRemoveSig *sig) {
  if (sig->id == NULL || strlen(sig->id) == 0) {
    return sig->callback(sig, PEOPLES_ID_EMPTY);
  }

  db_sql(sig->db, "delete people where id = $1::bigint");
  db_param(sig->db, sig->id);
  db_send(sig->db, sig, onPeoplesRemoveResp);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesDetailsResp(DB *db) {
  PeoplesDetailsSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  if (db_count(db) != 1) {
    return sig->callback(sig, PEOPLES_ERROR);
  }

  strcpy(sig->resp.id, db_value(db, 0, 0));
  strcpy(sig->resp.name, db_value(db, 0, 1));
  strcpy(sig->resp.email, db_value(db, 0, 2));

  return sig->callback(sig, PEOPLES_OK);
}

void peoples_details(PeoplesDetailsSig *sig) {
  if (sig->id == NULL || strlen(sig->id) == 0) {
    return sig->callback(sig, PEOPLES_ID_EMPTY);
  }

  db_sql(sig->db, "select id, name, email from people where id = $1::bigint");
  db_param(sig->db, sig->id);
  db_send(sig->db, sig, onPeoplesDetailsResp);
}

////////////////////////////////////////////////////////////////////////////////
