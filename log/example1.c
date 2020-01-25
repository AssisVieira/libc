#include "log.h"
#include <stdio.h>

int main() {
    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    return 0;
}
