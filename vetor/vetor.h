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

void vetor_destruir(Vetor *vetor);

void vetor_visitar(Vetor *vetor, FuncVisitante visitante);

size_t vetor_qtd(const Vetor *vetor);

void vetor_expandir(Vetor *vetor, size_t indice);

#endif
