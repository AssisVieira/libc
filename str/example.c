/*******************************************************************************
 *   Copyright 2019 Assis Vieira
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

#include "str.h"
#include <stdio.h>

int main() {
  str_t *nome = str_new(10);

  if (str_addcstr(&nome, "Fulano Beltrano")) {
    perror("Erro: str_addcstr().\n");
    return -1;
  }

  str_t *outroNome = str_clonecstr("João Silva");

  if (outroNome == NULL) {
    perror("Erro: str_clonecstr()\n");
    return -1;
  }

  if (str_cmp(nome, outroNome) == 0) {
    printf("'%s' é '%s'\n", str_cstr(nome), str_cstr(outroNome));
  } else {
    printf("'%s' não é '%s'\n", str_cstr(nome), str_cstr(outroNome));
  }

  str_free(&nome);
  str_free(&outroNome);

  return 0;
}
