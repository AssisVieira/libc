#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "caprixo.h"
#include "db/db.h"

#define OPCAO_NOME_MAX 128

#define clear() printf("\033[H\033[J")

static void control_sair() {
  printf("Tchau!\n");
  exit(0);
}

typedef struct Opcao {
  char nome[OPCAO_NOME_MAX];
  void (*handler)();
} Opcao;

static Opcao MENU[] = {
  {"Adicionar Produto", control_produto_add},
  {"Listar Produto",    control_produto_list},
  {"Remover Produto",   control_produto_del},
  {"Editar Produto",    control_produto_edit},
  {"Sair",              control_sair},
};

static void menu(const Opcao menu[], size_t size) {
  do {
    int opcao = -1;
  
    for (int i = 0; i < size; i ++) {
      printf("[%d] %s\n", i, menu[i].nome);
    }

    do {
      printf("\nOpcao: ");
      scanf("%d", &opcao);   
      while ((getchar()) != '\n');
    } while (opcao < 0 || opcao >= size);

    menu[opcao].handler();
  } while (true);
}

int main() {
  DBPool *pool = db_pool_create("host=127.0.0.1 user=assis password=assis "
                                "dbname=assis", false, 1, 1);

  control_produto_init(pool);

  menu(MENU, sizeof(MENU) / sizeof(Opcao));

  return 0;
} 

