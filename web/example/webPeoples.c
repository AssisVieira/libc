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
#include <string.h>

#include "log/log.h"
#include "peoples.h"
#include "str/str.h"

////////////////////////////////////////////////////////////////////////////////

void webPeoples_list(HttpClient *client) {
  const char *query = http_reqParam(client, "q");
  int page = http_reqParamInt(client, "p", 0);
  int pageSize = http_reqParamInt(client, "ps", 10);

  People peoples[PEOPLES_LIST_MAX_PEOPLES];  

  int count = peoples_list(query, page, pageSize, peoples);

  Json *body = json_new(10000);

  if (count >= 0) {
    json_addInt(body, "status", PEOPLES_OK);
    json_beginArray(body, "peoples");
    for (int i = 0; i < count; i++) {
      json_beginObject(body, "");
      json_add(body, "id", peoples[i].id);
      json_add(body, "name", peoples[i].name);
      json_add(body, "email", peoples[i].email);
      json_endObject(body);
    }
    json_endArray(body);
  } else {
    json_addInt(body, "status", PEOPLES_ERROR);
    json_beginObject(body, "error");
    json_add(body, "code", people_errorCode());
    json_add(body, "msg", people_errorMsg());
    json_endObject(body);
  }

  http_sendStatus(client, HTTP_STATUS_OK);
  http_sendType(client, HTTP_TYPE_JSON);
  http_send(client, json_cstr(body), json_len(body));

  json_free(&body);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesAddResp(PeoplesAddSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->client;

  if (status == PEOPLES_OK) {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"ok\"}", strlen("{\"status\": \"ok\"}"));
  } else {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"error\"}",
              strlen("{\"status\": \"error\"}"));
  }

  free(sig);
}

void webPeoples_add(HttpClient *client) {
  PeoplesAddSig *sig = malloc(sizeof(PeoplesAddSig));
  sig->name = http_reqParam(client, "name");
  sig->email = http_reqParam(client, "email");
  sig->callback = onPeoplesAddResp;
  sig->client = client;

  log_info("web-peoples", "body = %s\n", http_reqBody(client));

  peoples_add(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesRemoveResp(PeoplesRemoveSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->client;

  if (status == PEOPLES_OK) {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"ok\"}", strlen("{\"status\": \"ok\"}"));
  } else {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"error\"}",
              strlen("{\"status\": \"error\"}"));
  }

  free(sig);
}

void webPeoples_remove(HttpClient *client) {
  PeoplesRemoveSig *sig = malloc(sizeof(PeoplesRemoveSig));
  sig->id = http_reqParam(client, "id");
  sig->callback = onPeoplesRemoveResp;
  sig->client = client;

  peoples_remove(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesUpdateResp(PeoplesUpdateSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->client;

  if (status == PEOPLES_OK) {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"ok\"}", strlen("{\"status\": \"ok\"}"));
  } else {
    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, "{\"status\": \"error\"}",
              strlen("{\"status\": \"error\"}"));
  }

  free(sig);
}

void webPeoples_update(HttpClient *client) {
  PeoplesUpdateSig *sig = malloc(sizeof(PeoplesUpdateSig));
  sig->id = http_reqParam(client, "id");
  sig->name = http_reqParam(client, "name");
  sig->email = http_reqParam(client, "email");
  sig->callback = onPeoplesUpdateResp;
  sig->client = client;

  peoples_update(sig);
}

////////////////////////////////////////////////////////////////////////////////

static void onPeoplesDetailsResp(PeoplesDetailsSig *sig, PeoplesStatus status) {
  HttpClient *client = sig->client;

  if (status == PEOPLES_OK) {
    str_t *body = str_new(1000);

    str_fmt(body,
            "{\"status\": \"%s\", \"people\": {\"id\": \"%s\", \"name\": "
            "\"%s\", \"email\": \"%s\"}}",
            "ok", sig->resp.id, sig->resp.name, sig->resp.email);

    http_sendStatus(client, HTTP_STATUS_OK);
    http_sendType(client, HTTP_TYPE_JSON);
    http_send(client, str_cstr(body), str_len(body));

    str_free(&body);
  } else {
    http_sendError(client);
  }

  free(sig);
}

void webPeoples_details(HttpClient *client) {
  PeoplesDetailsSig *sig = malloc(sizeof(PeoplesDetailsSig));
  sig->id = http_reqArg(client, 0);
  sig->callback = onPeoplesDetailsResp;
  sig->client = client;

  peoples_details(sig);
}

////////////////////////////////////////////////////////////////////////////////
