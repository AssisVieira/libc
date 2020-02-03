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

#include "httpPeople.h"
#include "db/db.h"
#include "people.h"
#include <stdlib.h>

static void onPeopleSearchResp(PeopleSearchSig *sig, PeopleStatus status);

void httpPeople_addForm(HttpClient *client) {
  http_respOk(client, HTTP_TYPE_HTML, assets_get("people/addForm.html"));
}

void httpPeople_search(HttpClient *client) {
  PeopleSearchSig *sig = malloc(sizeof(PeopleSearchSig));
  sig->query = http_reqParam(client, "q");
  sig->page = 0;
  sig->pageSize = 10;
  sig->callback = onPeopleSearchResp;
  sig->ctx = client;
  sig->db = db_open();

  people_search(sig);
}

static void onPeopleSearchResp(PeopleSearchSig *sig, PeopleStatus status) {
  HttpClient *client = sig->ctx;

  if (status != PEOPLE_OK) {
    db_close(sig->db);
    free(sig);
    return http_respError(client);
  }

  http_respBegin(client, 200, HTTP_TYPE_HTML);
  http_respBody(client, "Buscando por: %s<br>\n", http_reqParam(client, "q"));

  for (int i = 0; i < sig->resp.peopleLen; i++) {
    http_respBody(client, "%s<br>\n", sig->resp.peoples[i].name);
  }

  http_respEnd(client);

  db_close(sig->db);
  free(sig);
}
