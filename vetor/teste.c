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
