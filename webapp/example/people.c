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

#include "people.h"
#include "db/db.h"
#include "log/log.h"
#include <string.h>

static void onPeopleSearchResp(DB *db);

void people_search(PeopleSearchSig *sig) {
  if (strlen(sig->query) == 0) {
    return sig->callback(sig, PEOPLE_QUERY_EMPTY);
  }

  if (sig->page < 0) {
    sig->page = 0;
  }

  if (sig->pageSize < 10) {
    sig->pageSize = 10;
  }

  db_sql(sig->db, "select id, name, email from people where name ilike '%' || "
                  "$1::text || '%' "
                  "OR email ilike '%' || $1::text || '%'");

  db_param(sig->db, sig->query);

  db_send(sig->db, sig, onPeopleSearchResp);
}

static void onPeopleSearchResp(DB *db) {
  PeopleSearchSig *sig = db_context(db);

  if (db_error(db)) {
    return sig->callback(sig, PEOPLE_ERROR);
  }

  sig->resp.peopleLen = db_count(db);

  for (int i = 0; i < db_count(db); i++) {
    strcpy(sig->resp.peoples[i].id, db_value(db, i, 0));
    strcpy(sig->resp.peoples[i].name, db_value(db, i, 1));
    strcpy(sig->resp.peoples[i].email, db_value(db, i, 2));
  }

  return sig->callback(sig, PEOPLE_OK);
}
