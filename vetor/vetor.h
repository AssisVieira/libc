#ifndef VETOR_H
#define VETOR_H

#include <assert.h>
#include <stdlib.h>

typedef struct Vetor {
  void **itens;
  size_t qtd;
  size_t capacidade;
} Vetor;

typedef int (*FuncVisitante)(void **obj);

Vetor *vetor_criar(size_t capacidadeInicial);

int vetor_init(Vetor *vetor, size_t capacidadeInicial);

void *vetor_item(const Vetor *vetor, size_t indice);

size_t vetor_add(Vetor *vetor, void *objeto);

void vetor_inserir(Vetor *vetor, size_t indice, void *objeto);

int vetor_destruir(Vetor *vetor);

void vetor_visitar(Vetor *vetor, FuncVisitante visitante);

size_t vetor_qtd(const Vetor *vetor);

void vetor_expandir(Vetor *vetor, size_t indice);

#endif
