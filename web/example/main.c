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

#include "web/web.h"
#include "webPeoples.h"

int main() {
  // Module People
  web_handler("GET", "/peoples$", webPeoples_list);
  web_handler("POST", "/peoples$", webPeoples_add);
  web_handler("DELETE", "/peoples/(.*)$", webPeoples_remove);
  web_handler("PUT", "/peoples/(.*)$", webPeoples_update);
  web_handler("GET", "/peoples/(.*)$", webPeoples_details);

  // Assets
  web_assets("/(.*)$", "public/");
  web_assets("/favicon.ico$", "public/imgs/favicon/favicon.ico");

  // Redirect
  web_redirect("/favicon.ico$", "public/imgs/favicon/favicon.ico");

  return web_start();
}
