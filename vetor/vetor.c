#include "vetor.h"

////////////////////////////////////////////////////////////////////////////////
// VISITAR CADA ITEM
////////////////////////////////////////////////////////////////////////////////

void vetor_visitar(Vetor *vetor, FuncVisitante visitante) {
  assert(vetor != NULL);
  assert(visitante != NULL);

  for (size_t i = 0; i < vetor->qtd; i++) {
    if (visitante(&vetor->itens[i]) == -1)
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// ADICIONAR ITEM
////////////////////////////////////////////////////////////////////////////////

static size_t proximoIndiceLivre(const Vetor *vetor) { return vetor->qtd; }

size_t vetor_add(Vetor *vetor, void *objeto) {
  size_t indice = proximoIndiceLivre(vetor);
  vetor_inserir(vetor, indice, objeto);
  return indice;
}

////////////////////////////////////////////////////////////////////////////////
// CRIAR VETOR
////////////////////////////////////////////////////////////////////////////////

Vetor *vetor_criar(size_t capacidadeInicial) {
  Vetor *vetor = malloc(sizeof(Vetor));

  if (vetor_init(vetor, capacidadeInicial) != 0)
    return NULL;

  return vetor;
}

////////////////////////////////////////////////////////////////////////////////
// DESTRUIR VETOR
////////////////////////////////////////////////////////////////////////////////

int vetor_destruir(Vetor *vetor) {
  assert(vetor != NULL);
  vetor->capacidade = 0;
  vetor->qtd = 0;
  free(vetor->itens);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// EXPANDIR VETOR
////////////////////////////////////////////////////////////////////////////////

static void aumentarCapacidadeNecessaria(Vetor *vetor, size_t indice) {
  size_t capacidadeAntiga = vetor->capacidade;

  while (indice >= vetor->capacidade) {
    if (vetor->capacidade > 1)
      vetor->capacidade = (size_t)((double)vetor->capacidade * 1.75);
    else
      vetor->capacidade = 2;
  }

  vetor->itens = realloc(vetor->itens, sizeof(void *) * vetor->capacidade);

  for (size_t i = capacidadeAntiga; i < vetor->capacidade; i++)
    vetor->itens[i] = NULL;
}

static void aumentarQtdNecessaria(Vetor *vetor, size_t indice) {
  vetor->qtd = indice + 1;
}

void vetor_expandir(Vetor *vetor, size_t indice) {
  if (indice >= vetor->capacidade)
    aumentarCapacidadeNecessaria(vetor, indice);

  if (indice >= vetor->qtd)
    aumentarQtdNecessaria(vetor, indice);
}

////////////////////////////////////////////////////////////////////////////////
// INICIALIZAR VETOR
////////////////////////////////////////////////////////////////////////////////

int vetor_init(Vetor *vetor, size_t capacidadeInicial) {
  vetor->qtd = 0;
  vetor->capacidade = capacidadeInicial;
  vetor->itens = malloc(sizeof(void *) * capacidadeInicial);

  if (vetor->itens == NULL)
    return -1;

  for (size_t i = 0; i < vetor->capacidade; i++)
    vetor->itens[i] = NULL;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// INSERIR ITEM
////////////////////////////////////////////////////////////////////////////////

void vetor_inserir(Vetor *vetor, size_t indice, void *objeto) {
  vetor_expandir(vetor, indice);
  vetor->itens[indice] = objeto;
}

////////////////////////////////////////////////////////////////////////////////
// OBTER ITEM
////////////////////////////////////////////////////////////////////////////////

void *vetor_item(const Vetor *vetor, size_t indice) {
  if (indice >= vetor->qtd)
    return NULL;
  return vetor->itens[indice];
}

////////////////////////////////////////////////////////////////////////////////
// QUANTIDADE DE ELEMENTOS
////////////////////////////////////////////////////////////////////////////////

size_t vetor_qtd(const Vetor *vetor) { return vetor->qtd; }
