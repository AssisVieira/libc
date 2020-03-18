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

#include "db/db.h"
#include <stdio.h>
#include <string.h>

int main() {
  const char strConn[] = "postgresql://assis:assis@127.0.0.1:5432/assis";
  DBPool *pool = NULL;
  DB *db = NULL;

  pool = db_pool_create(strConn, false, 1, 5);

  db = db_pool_get(pool);

  db_sql(db, "insert into people (name, email) values ($1::text, $2::text)");
  db_param(db, "Jhon");
  db_param(db, "jhon@provider.com");
  
  if (db_exec(db)) {
    perror("error: db_exec().\n");
  } else {
    printf("Row inserted!\n");
  }

  db_close(db);

  db_pool_destroy(pool);
  
  return 0;
}
