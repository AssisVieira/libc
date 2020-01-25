#include "log.h"
#include <stdio.h>

int main() {
    if (log_open("/home/assis/log.txt")) {
        perror("Erro na abertura do log.\n");
        return -1;
    }

    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    if (log_close()) {
        perror("Erro ao encerrar o log.");
        return -1;
    }

    return 0;
}
