/*******************************************************************************
 *   Copyright 2019 Assis Vieira
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

/**
 * Módulo de log
 * =============
 *
 * Registra logs informativos, de atenção, de erros ou de debug do sistema. Este
 * módulo representa uma única instância para todo o sistema.
 *
 * Caso de uso 1: registrando as mensagens de log apenas no console.
 * --------------
 *
 * #include "log/log.h"
 * #include <stdio.h>
 *
 * int main() {
 *     log_info("meu-modulo", "minha %s.\n", "mensagem informativa");
 *
 *     log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");
 *
 *     log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");
 *
 *     log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");
 *
 *     return 0;
 * }
 *
 *
 * Caso de uso 2: registrando as mensagens de log no disco e no console.
 * --------------
 *
 * #include "log/log.h"
 * #include <stdio.h>
 *
 * int main() {
 *     if (log_open("log.txt")) {
 *         perror("Erro na abertura do log.\n");
 *         return -1;
 *     }
 *
 *     log_info("meu-modulo", "minha %s.\n", "mensagem informativa");
 *
 *     log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");
 *
 *     log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");
 *
 *     log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");
 *
 *     if (log_close()) {
 *         perror("Erro ao encerrar o log.");
 *         return -1;
 *     }
 *
 *     return 0;
 * }
 *
 *
 * Caso de uso 3: registrando as mensagens de log apenas no disco.
 * --------------
 *
 * #include "log/log.h"
 * #include <stdio.h>
 *
 * int main() {
 *     if (log_open("log.txt")) {
 *         perror("Erro na abertura do log.\n");
 *         return -1;
 *     }
 *
 *     log_terminal(false);
 *
 *     log_info("meu-modulo", "minha %s.\n", "mensagem informativa");
 *
 *     log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");
 *
 *     log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");
 *
 *     log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");
 *
 *     if (log_close()) {
 *         perror("Erro ao encerrar o log.");
 *         return -1;
 *     }
 *
 *     return 0;
 * }
 *
 * @author assis.sv@gmail.com
 */
#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stddef.h>

#define LOG_IGNORE_MAX 255
#define LOG_MODULE_NAME_MAX 32

/**
 * Níveis de granularidade do log.
 */
typedef enum LogLevel {
  LOG_ERRO = 0,
  LOG_WARN,
  LOG_INFO,
  LOG_TRAC,
  LOG_DBUG,
} LogLevel;

/**
 * Protótipo da função que deve ser implemetada pelo filtro de log.
 *
 * Veja mais nos comentários da função log_filter().
 *
 * @param  module nome do módulo. A mesma string passada quando as funções
 *                log_info, log_debug ou log_error são chamadas.
 * @param  fmt    string de formatação do log. A mesma string passada quando as
 *                funções log_info, log_debug ou log_error são chamadas.
 * @return        true, se o log deve ser filtrado, false, caso contrário.
 */
typedef bool (*LogFilter)(const char *module, const char *fmt);

/**
 * Abre o arquivo de log.
 *
 * Após chamar esta função, todos os logs serão registrados em arquivo no disco.
 *
 * A chamada desta função é opcional. Se a função não for chamada, os logs *não*
 * serão registrados no disco. Se a função for invocada, logo, a função
 * log_close() também deverá ser invocada a fim de fechar o arquivo aberto por
 * esta função.
 *
 * Preferencialmente, se o registro do log do sistema em arquivo for um
 * requisito, a sugestão é para que as funções log_open() e log_close() sejam
 * invocadas no início e no fim do método main(), respectivamente.
 *
 * @param  path     endereço do arquivo do log.
 * @return          0, em caso de sucesso, -1 em caso de erro na abertura do
 *                  arquivo para escrita.
 */
int log_open(const char *path);

/**
 * Fecha o arquivo de log aberto pela função log_open();
 *
 * Esta função deve sempre ser chamada apenas quando a função log_open() for
 * invocada.
 *
 * @return 0, em caso de fechamento com sucesso, -1, em caso de problemas
 *         no fechamento, por exemplo, espaço insuficiente no disco.
 */
int log_close();

/**
 * Registra log informativo sobre o sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_info(const char *module, const char *fmt, ...);

/**
 * Registra log de atenção sobre o sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_warn(const char *module, const char *fmt, ...);

/**
 * Registra log de trace do sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_trac(const char *module, const char *fmt, ...);

/**
 * Registra log de depuração sobre o sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_dbug(const char *module, const char *fmt, ...);

/**
 * Registra log de depuração sobre o sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * Esta função realiza o log de buffs.
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param buff    vetor representando o buff.
 * @param size    quantidade de bytes do buff.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_dbugbin(const char *module, const char *buff, size_t size,
                 const char *fmt, ...);

/**
 * Registra log de erro sobre o sistema.
 *
 * A formatação da mensagem segue a mesma especificação da função fprintf().
 *
 * @param module  nome do módulo que está originando o registro do log.
 * @param fmt     string de formatação da mensagem do log.
 * @param VARARGS argumentos a serem usados na formatação da mensagem do log.
 */
void log_erro(const char *module, const char *fmt, ...);

/**
 * Filtra as mensagens de log, independente do canal de saída, seja disco
 * ou console.
 *
 * Esta é uma função opcional. O comportamente padrão é que nenhum log seja
 * filtrado.
 *
 * @param  filter Uma função que será invocada a cada registro de log. A função
 *                deve receber duas strings como argumento: o nome do módulo e a
 *                string de formatação do log. Se o log deve ser ignorado, isto
 *                é, filtrado, logo a função deverá retorna true, caso
 *                contrário, deverá retornar false. Se desejar remover qualquer
 *                filtro configurado anteriormente, apenas informe NULL.
 */
void log_filter(LogFilter filter);

/**
 * Ignora logs de um módulo de acordo com o nível de granularidade.
 *
 * @param module [description]
 * @param level  [description]
 */
void log_ignore(const char *module, LogLevel level);

/**
 * Determina se o log deve ser exibido nas saídas padrão do console: stdout e
 * stderr.
 *
 * Esta função é opcional. O comportamente padrão é que todos os logs sejam
 * exibidos no console.
 *
 * @param  terminal true, caso o logs devam ser exibidos no console, false, caso
 *                  contrário.
 */
void log_terminal(bool terminal);

#endif
