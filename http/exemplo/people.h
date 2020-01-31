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

#ifndef PEOPLE_H
#define PEOPLE_H

#include "db/db.h"
#include <stddef.h>

#define PEOPLE_SEARCH_MAX_PEOPLES 25

typedef struct People {
  char name[128];
  char email[128];
  char id[32];
} People;

typedef enum PeopleStatus {
  PEOPLE_OK = 0,
  PEOPLE_QUERY_EMPTY,
  PEOPLE_ERROR,
} PeopleStatus;

typedef struct PeopleSearchSig PeopleSearchSig;

typedef void (*PeopleSearchCb)(PeopleSearchSig *sig, PeopleStatus status);

struct PeopleSearchSig {
  const char *query;
  int page;
  int pageSize;
  DB *db;
  PeopleSearchCb callback;
  void *ctx;
  struct {
    int peopleLen;
    People peoples[PEOPLE_SEARCH_MAX_PEOPLES];
  } resp;
};

void people_search(PeopleSearchSig *sig);

#endif
