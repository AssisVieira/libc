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

#ifndef WEB_H
#define WEB_H

#include "http/http.h"

typedef void (*WebHandler)(HttpClient *client);

void web_handler(const char *method, const char *path, WebHandler handler);

int web_start();

void web_close(int result);

void web_assets(const char *from, const char *to);

void web_redirect(const char *from, const char *to);

#endif
