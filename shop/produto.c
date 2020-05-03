#include <stdlib.h>
#include <string.h>
#include "produto.h"

static DBPool *gPool = NULL;

void produto_init(DBPool *pool) {
  gPool = pool;
}

Produto *produto_new(const char *nome) { 
  Produto *produto = malloc(sizeof(Produto));
  strncpy(produto->nome, nome, PROD_NOME_SIZE);
  return produto;
}

void produto_free(Produto *produto) {
  free(produto);
}

int produto_add(const Produto *produto) {
  DB* db = db_pool_get(gPool);

  db_sql(db, 
      "insert into produto (codigo, nome, precoTabela, preco, parcelas,"
      "descricao) values ($1::text, $2::text, $3::numeric(12,2),       "
      "$4::numeric(12,2), $5::int, $6::text);");

  db_param(db, produto->codigo);
  db_param(db, produto->nome);
  db_paramDouble(db, produto->precoTabela);
  db_paramDouble(db, produto->preco);
  db_paramInt(db, produto->parcelas);
  db_param(db, produto->descricao);

  db_exec(db);

  return db_close(db);
}

int produto_buscar(Produto *produtos, int pg, int pgSize) {
  int count = 0;
  DB *db = db_pool_get(gPool);

  db_sql(db, "select codigo, nome, precoTabela, preco, parcelas, descricao "
             "from produto OFFSET $1::bigint LIMIT $2::bigint;");

  db_paramInt(db, pg);
  db_paramInt(db, pgSize);

  db_exec(db);
  
  for (int i = 0; i < db_count(db); i++) {
    strncpy(produtos[i].codigo, db_value(db, i, 0), PROD_CODI_SIZE);
    strncpy(produtos[i].nome, db_value(db, i, 1), PROD_NOME_SIZE);
    produtos[i].precoTabela = db_valueDouble(db, i, 2);
    produtos[i].preco = db_valueDouble(db, i, 3);
    produtos[i].parcelas = db_valueInt(db, i, 4);
    strncpy(produtos[i].descricao, db_value(db, i, 5), PROD_DESC_SIZE);
    count++;     
  }

  if (db_close(db)) {
    return -1;
  }

  return count;
}



