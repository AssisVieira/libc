#ifndef PRODUTO_H
#define PRODUTO_H

#include "db/db.h"

typedef struct Produto {
  char codigo[128];
  char nome[128];
  double precoTabela;
  double preco;
  int parcelas;
  char descricao[4096];
} Produto;

void produto_init(DBPool *pool);

Produto *produto_new(const char *nome);

/**
 * Adiciona um produto na loja.
 */
int produto_add(const Produto *produto);

/**
 * Buscar produtos na loja.
 */
int produto_buscar(Produto *produtos, int pg, int pgSize);

void produto_free(Produto *produto);

#endif

