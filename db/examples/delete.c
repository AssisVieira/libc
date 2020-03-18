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
  const int idPeople = 10;
  DB *db = NULL;  

  db = db_open(strConn, false);

  db_sql(db, "delete from people where id = $1::integer");
  db_paramInt(db, idPeople);
  
  if (db_exec(db)) {
    perror("error: db_exec().\n");
  } else {
    printf("Row deleted!\n");
  }

  db_close(db);
  
  return 0;
}
