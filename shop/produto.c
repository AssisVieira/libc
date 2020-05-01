#include <stdlib.h>
#include <string.h>
#include "produto.h"

static DBPool *gPool = NULL;

void produto_init(DBPool *pool) {
  gPool = pool;
}

Produto *produto_new(const char *nome) { 
  Produto *produto = malloc(sizeof(Produto));
  strncpy(produto->nome, nome, sizeof(produto->nome));
  return produto;
}

void produto_free(Produto *produto) {
  free(produto);
}

int produto_add(const Produto *produto) {
  int r = 0;

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

  if (db_exec(db)) {
    r = -1;
  }

  db_close(db);

  return r;
}

int produto_buscar(Produto *produtos, int pg, int pgSize) {
  int count = 0;
  DB *db = db_pool_get(gPool);

  db_sql(db, "select codigo, nome, precoTabela, preco, parcelas, descricao "
             "from produto OFFSET $1::bigint LIMIT $2::bigint;");

  db_paramInt(db, pg);
  db_paramInt(db, pgSize);

  if (db_exec(db)) {
    count = -1;
  } else {
    for (int i = 0; i < db_count(db); i++) {
      strncpy(produtos[i].codigo, db_value(db, i, 0), sizeof(produtos[i].codigo));
      strncpy(produtos[i].nome, db_value(db, i, 1), sizeof(produtos[i].nome));
      produtos[i].precoTabela = db_valueDouble(db, i, 2);
      produtos[i].preco = db_valueDouble(db, i, 3);
      produtos[i].parcelas = db_valueInt(db, i, 4);
      strncpy(produtos[i].descricao, db_value(db, i, 5), sizeof(produtos[i].descricao));

      count++;     

      if (db_error(db)) {
        count = -1;
        break;
      }
    }
  }

  db_close(db);  

  return count;
}



