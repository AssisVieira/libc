#include <stdio.h>
#include <string.h>
#include "produto.h"
#include "control_produto.h"

static int read_str(const char *label, char *var, size_t max) {
  int r = 0;
  do {
    char format[16] = {0};
    char tmp[max];
    printf("%s [%s]: ", label, var);
    snprintf(format, sizeof(format), "%%%zus", max - 1);
    r = scanf(format, tmp);
    while (getchar() != '\n');
    if (r != EOF && strlen(tmp) > 0) {
      memcpy(var, tmp, max);
    }
  } while (r == EOF);
  return r;
}

static int read_int(const char *label, int *num) {
  int r = 0;
  do {
    int tmp = 0;
    printf("%s [%d]: ", label, *num);
    r = scanf("%d", &tmp);
    while (getchar() != '\n');
    if (r != EOF) {
      *num = tmp;
    }
  } while (r == EOF);
  return r;
}

static int read_double(const char *label, double *num) {
  int r = 0;
  do {
    double tmp = 0;
    printf("%s [%lf]: ", label, *num);
    r = scanf("%lf", &tmp);
    while (getchar() != '\n');
    if (r != EOF) {
      *num = tmp;
    }
  } while (r == EOF);
  return r;
}

void control_produto_init(DBPool *pool) {
  produto_init(pool);
}

void control_produto_add() {
  Produto produto = {0};

  printf("Adicionar produto\n\n");

  read_str("Código de Referência", produto.codigo, sizeof(produto.codigo));
  read_str("Nome", produto.nome, sizeof(produto.nome));
  read_double("Preço de tabela", &produto.precoTabela);
  read_double("Preço", &produto.preco);
  read_int("Parcelas", &produto.parcelas);
  read_str("Descrição", produto.descricao, sizeof(produto.descricao));

  if (!produto_add(&produto)) {
    printf("Produto adicionado.\n");
  } else {
    perror("Erro ao adicionar produto.");
  }
}

void control_produto_list() { 
  Produto produtos[10] = {0};
  int count;

  count = produto_buscar(produtos, 0, 10);

  if (count < 0) {
    printf("Erro ao obter produtos.\n");
    return;
  } else if (count == 0) {
    printf("Nenhum produto cadastrado.\n");
    return;
  }

  for (int i = 0; i < count; i++) {
    printf("#%s - %s (R$ %.2lf)\n", produtos[i].codigo, 
        produtos[i].nome, 
        produtos[i].preco);
  }
  
}

void control_produto_edit() { 
  printf("Editar produto\n");
}

void control_produto_del() { 
  printf("Remover produto\n");
}


