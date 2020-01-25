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
#include "people.h"

static void onSearchResult(void *context, People peoples[], size_t len);

void httpPeople_searchView(HttpClient *client) {
  http_respOk(client, HTTP_TYPE_HTML, "search view");
}

void httpPeople_search(HttpClient *client) {
  const char *query = "term";
  int page = 25;
  int pageSize = 10;

  if (people_search(query, page, pageSize, onSearchResult, client)) {
    return;
  }
}

static void onSearchResult(void *context, People peoples[], size_t len) {
  HttpClient *client = context;

  http_respBegin(client, 200, HTTP_TYPE_HTML);

  http_respBody(client, "Buscando por: %s<br>\n", http_reqParam(client, "q"));

  for (int i = 0; i < len; i++) {
    http_respBody(client, "%s<br>\n", peoples[i].name);
  }

  http_respEnd(client);
}
