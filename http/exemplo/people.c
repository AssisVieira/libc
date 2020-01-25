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

int people_search(const char *query, int page, int pageSize,
                  onPeopleSearchResult callback, void *ctx) {

  People people[] = {
      {"Fulano", 33},
      {"Beltrano", 31},
      {"Cicrano", 29},
  };

  callback(ctx, people, sizeof(people) / sizeof(People));

  return 0;
}
