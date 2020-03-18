#include "db.h"

static void testOpen() {
  // db_open() deve retornar NULL, caso a string de conexão for nula.
  assert(db_open(NULL, false) == NULL);

  // db_open() deve retornar uma instância de DB, caso a string de conexão
  // estiver correta e uma conexão for realizada com o banco.
  assert(db_open("postgresql://assis:assis@127.0.0.1:5432/assis", false) !=
         NULL);
}

int main() {
  testOpen();
  return 0;
}
