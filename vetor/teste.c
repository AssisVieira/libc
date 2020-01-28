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
 
#include "vetor/vetor.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main() {
  char *obj0 = "palavra0";
  char *obj1 = "palavra1";
  char *obj2 = "palavra2";
  char *obj3 = "palavra3";

  Vetor *vetor = vetor_criar(1);

  vetor_add(vetor, obj0);
  vetor_add(vetor, obj1);
  vetor_add(vetor, obj2);

  assert(vetor_qtd(vetor) == 3);

  assert(vetor_item(vetor, 0) == obj0);
  assert(vetor_item(vetor, 1) == obj1);
  assert(vetor_item(vetor, 2) == obj2);

  vetor_inserir(vetor, 0, obj1);
  vetor_inserir(vetor, 1, obj2);
  vetor_inserir(vetor, 2, obj0);
  vetor_inserir(vetor, 3, obj3);

  assert(vetor_item(vetor, 0) == obj1);
  assert(vetor_item(vetor, 1) == obj2);
  assert(vetor_item(vetor, 2) == obj0);
  assert(vetor_item(vetor, 3) == obj3);

  vetor_destruir(vetor);

  assert(strcmp(obj0, "palavra0") == 0);
  assert(strcmp(obj1, "palavra1") == 0);
  assert(strcmp(obj2, "palavra2") == 0);
  assert(strcmp(obj3, "palavra3") == 0);

  printf("vetor: Ok\n");

  return 0;
}
