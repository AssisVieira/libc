/*******************************************************************************
 *   Copyright 2020 Assis Vieira
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************/
 
#include "log/log.h"
#include <stdio.h>

int main() {
    if (log_open("log.txt")) {
        perror("Erro na abertura do log.\n");
        return -1;
    }

    log_terminal(false);

    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    log_terminal(true);

    if (log_close()) {
        perror("Erro ao encerrar o log.");
        return -1;
    }

    return 0;
}
