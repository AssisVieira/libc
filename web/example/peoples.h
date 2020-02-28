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

#ifndef PEOPLES_H
#define PEOPLES_H

#include <stddef.h>

#include "db/db.h"

#define PEOPLES_LIST_MAX_PEOPLES 25

typedef struct People {
  char name[128];
  char email[128];
  char id[32];
} People;

typedef enum PeoplesStatus {
  PEOPLES_OK = 0,
  PEOPLES_QUERY_EMPTY,
  PEOPLES_ID_EMPTY,
  PEOPLES_NAME_EMPTY,
  PEOPLES_EMAIL_EMPTY,
  PEOPLES_ERROR,
} PeoplesStatus;

typedef struct PeoplesListSig PeoplesListSig;
typedef void (*PeoplesListCb)(PeoplesListSig *sig, PeoplesStatus status);
struct PeoplesListSig {
  const char *query;
  int page;
  int pageSize;
  DB *db;
  PeoplesListCb callback;
  void *ctx;
  struct {
    int peopleLen;
    People peoples[PEOPLES_LIST_MAX_PEOPLES];
  } resp;
};

typedef struct PeoplesAddSig PeoplesAddSig;
typedef void (*PeoplesAddCb)(PeoplesAddSig *sig, PeoplesStatus status);
struct PeoplesAddSig {
  const char *name;
  const char *email;
  DB *db;
  PeoplesAddCb callback;
  void *ctx;
  struct {
  } resp;
};

typedef struct PeoplesRemoveSig PeoplesRemoveSig;
typedef void (*PeoplesRemoveCb)(PeoplesRemoveSig *sig, PeoplesStatus status);
struct PeoplesRemoveSig {
  const char *id;
  DB *db;
  PeoplesRemoveCb callback;
  void *ctx;
  struct {
  } resp;
};

typedef struct PeoplesUpdateSig PeoplesUpdateSig;
typedef void (*PeoplesUpdateCb)(PeoplesUpdateSig *sig, PeoplesStatus status);
struct PeoplesUpdateSig {
  const char *id;
  const char *name;
  const char *email;
  DB *db;
  PeoplesUpdateCb callback;
  void *ctx;
  struct {
  } resp;
};

typedef struct PeoplesDetailsSig PeoplesDetailsSig;
typedef void (*PeoplesDetailsCb)(PeoplesDetailsSig *sig, PeoplesStatus status);
struct PeoplesDetailsSig {
  const char *id;
  DB *db;
  PeoplesDetailsCb callback;
  void *ctx;
  People resp;
};

void peoples_list(PeoplesListSig *sig);
void peoples_update(PeoplesUpdateSig *sig);
void peoples_add(PeoplesAddSig *sig);
void peoples_remove(PeoplesRemoveSig *sig);
void peoples_details(PeoplesDetailsSig *sig);

#endif
