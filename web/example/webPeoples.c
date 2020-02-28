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

#include "webPeoples.h"

#include <stdlib.h>

#include "db/db.h"
#include "log/log.h"
#include "peoples.h"
#include "str/str.h"

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesListResp(PeoplesListSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->ctx;
  str_t *body = http_respBody(client);

  if (status == PEOPLES_OK) {
    str_fmt(&body, "{\"status\": \"%d\", \"peoples\": [", status);

    for (int i = 0; i < sig->resp.peopleLen; i++) {
      str_fmt(&body, "{\"id\": \"%s\", \"name\": \"%s\", \"email\": \"%s\"}%c",
              sig->resp.peoples[i].id, sig->resp.peoples[i].name,
              sig->resp.peoples[i].email,
              (i + 1 == sig->resp.peopleLen) ? ' ' : ',');
    }

  } else {
    str_fmt(&body, "{\"status\": \"%d\"}", status);
  }

  http_respSend(client, body);

  db_close(sig->db);

  free(sig);
}

void webPeoples_list(HttpClient *client) {
  PeoplesListSig *sig = malloc(sizeof(PeoplesListSig));
  sig->query = http_reqParam(client, "q");
  sig->page = 0;
  sig->pageSize = 10;
  sig->callback = onPeoplesListResp;
  sig->ctx = client;
  sig->db = db_open();

  peoples_list(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesAddResp(PeoplesAddSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->ctx;

  if (status == PEOPLES_OK) {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"ok\"}");
  } else {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"error\"}");
  }

  db_close(sig->db);
  free(sig);
}

void webPeoples_add(HttpClient *client) {
  PeoplesAddSig *sig = malloc(sizeof(PeoplesAddSig));
  sig->name = http_reqBodyString(client, "name");
  sig->email = http_reqBodyString(client, "email");
  sig->callback = onPeoplesAddResp;
  sig->ctx = client;
  sig->db = db_open();

  log_info("web-peoples", "body = %s\n", http_reqBody(client));

  peoples_add(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesRemoveResp(PeoplesRemoveSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->ctx;

  if (status == PEOPLES_OK) {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"ok\"}");
  } else {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"error\"}");
  }

  db_close(sig->db);
  free(sig);
}

void webPeoples_remove(HttpClient *client) {
  PeoplesRemoveSig *sig = malloc(sizeof(PeoplesRemoveSig));
  sig->id = http_reqParam(client, "id");
  sig->callback = onPeoplesRemoveResp;
  sig->ctx = client;
  sig->db = db_open();

  peoples_remove(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesUpdateResp(PeoplesUpdateSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->ctx;

  if (status == PEOPLES_OK) {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"ok\"}");
  } else {
    http_respOk(client, HTTP_TYPE_JSON, "{\"status\": \"error\"}");
  }

  db_close(sig->db);
  free(sig);
}

void webPeoples_update(HttpClient *client) {
  PeoplesUpdateSig *sig = malloc(sizeof(PeoplesUpdateSig));
  sig->id = http_reqJsonLong(client, "id");
  sig->name = http_reqJsonString(client, "name");
  sig->email = http_reqJsonString(client, "email");
  sig->callback = onPeoplesUpdateResp;
  sig->ctx = client;
  sig->db = db_open();

  peoples_update(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesDetailsResp(PeoplesDetailsSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->ctx;

  if (status == PEOPLES_OK) {
    str_t *body = str_new(1000);

    str_fmt(&body,
            "{\"status\": \"%s\", \"people\": {\"id\": \"%s\", \"name\": "
            "\"%s\", \"email\": \"%s\"}}",
            "ok", sig->resp.id, sig->resp.name, sig->resp.email);

    http_respBegin(client, HTTP_STATUS_OK, HTTP_TYPE_JSON);
    http_respBody(client, str_cstr(body), str_len(body));
    http_respEnd();

    str_free(&body);
  } else {
    http_respError(client);
  }

  db_close(sig->db);
  free(sig);
}

void webPeoples_details(HttpClient *client) {
  PeoplesDetailsSig *sig = malloc(sizeof(PeoplesDetailsSig));
  sig->id = http_reqArg(client, 0);
  sig->callback = onPeoplesDetailsResp;
  sig->ctx = client;
  sig->db = db_open();

  peoples_details(sig);
}

////////////////////////////////////////////////////////////////////////////////
