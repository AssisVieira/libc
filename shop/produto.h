#ifndef PRODUTO_H
#define PRODUTO_H

#include "db/db.h"

#define PROD_CODI_SIZE 128
#define PROD_NOME_SIZE 128
#define PROD_DESC_SIZE 4096

typedef struct Produto {
  char codigo[PROD_CODI_SIZE];
  char nome[PROD_NOME_SIZE];
  double precoTabela;
  double preco;
  int parcelas;
  char descricao[PROD_DESC_SIZE];
} Produto;

/**
 * Inicializa o módulo produto.
 *
 * @param pool pool de conexões com o banco de dados.
 */
void produto_init(DBPool *pool);

/**
 * Cria uma nova instância de produto.
 */
Produto *produto_new(const char *nome);

/**
 * Adiciona um produto na loja.
 *
 * @param produto produto a ser adicionado na loja.
 * @return 0, em caso de sucesso, 1, em caso de erro.
 */
int produto_add(const Produto *produto);

/**
 * Buscar produtos na loja.
 */
int produto_buscar(Produto *produtos, int pg, int pgSize);

/**
 * Destroi uma instância de produto.
 */ 
void produto_free(Produto *produto);

#endif

