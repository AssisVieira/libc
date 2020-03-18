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

#ifndef DB_H
#define DB_H

#include <stdbool.h>

#include "io/io.h"

#define DB_SQL_PARAMS_MAX 32

typedef struct DB DB;

typedef void (*onDBCallback)(DB *db);

typedef struct DBPool DBPool;

/**
 * Cria um o pool de conexões.
 *
 * @param  strConn  string de conexão.
 * @param  async    true, se as conexões devem ser assíncronas, false, caso
 *                  contrário.
 * @param  max      quantidade máxima de conexões ativas.
 * @param  min      quantidade mínima de conexões ativas.
 * @return          uma instancia de pool de conexões, em caso de sucesso, NULL,
 *                  em caso de erro.
 */
DBPool *db_pool_create(const char *strConn, bool async, int min, int max);

/**
 * Destroi o pool de conexões, fechando todas as conexões ativas e liberando
 * toda a memória utilizada pelo pool.
 */
void db_pool_destroy(DBPool *dbpool);

/**
 * Obtém uma conexão do pool.
 *
 * @return conexão com o banco de dados.
 */
DB *db_pool_get(DBPool *dbpool);

/**
 * Cria uma conexão com um banco de dados.
 *
 * @param  strConn  string de conexão.
 * @param  async    true, se a conexão deve ser assíncrona, false, caso contrário.
 */
DB *db_open(const char *strConn, bool async);

/**
 * Desfaz tudo o que ocorreu no bloco da transação atual e inicia uma nova
 * transação.
 */
int db_rollback(DB *db);

/**
 * Efetiva tudo o que ocorreu no bloco da transação atual e inicia uma nova
 * transação.
 */
int db_commit(DB *db);

/**
 * Efetiva tudo o que ocorreu no block da transação atual e, se a conexão foi
 * obtida de um pool, logo a conexão será mantida aberta, sendo apenas devolvida
 * ao pool. Se a conexão não foi obtida de um pool, logo a conexão será fechada
 * imediatamente, liberando toda a memória usada pela conexão.
 *
 * @param db conexão a ser devolvida.
 */
void db_close(DB *db);

/**
 * Configura o comando sql a ser executado quando db_send() ou db_exec() for
 * chamado.
 *
 * @param db  conexão com o banco de dados.
 * @param sql comando sql.
 */
void db_sql(DB *db, const char *sql);

/**
 * Libera toda a memória utilizada na execução do comando sql, após as chamadas
 * das funções db_send() ou db_exec().
 *
 * @param db conexão com o banco de dados.
 */
void db_clear(DB *db);

/**
 * Configura o valor de um parâmetro usado no comando sql.
 *
 * @param db    conexão com o banco de dados.
 * @param value valor do parâmetro.
 */
void db_param(DB *db, const char *value);
void db_paramInt(DB *db, int value);

/**
 * Executa o comando sql de forma assíncrona. Esta função deve ser sempre
 * chamada após a configuração da sql, com a função db_sql() e após a
 * configurações dos parâmetros da sql, se houver, com a função db_param().
 *
 * @param db          conexão com banco de dados.
 * @param context     objeto a ser passado para a callback.
 * @param onCmdResult callback para tratar a resposta do comando sql.
 */
void db_send(DB *db, void *context, onDBCallback onCmdResult);

/**
 * Envia o comando sql de forma síncrona. Esta função deve ser sempre
 * chamada após a configuração da sql, com a função db_sql() e após a
 * configurações dos parâmetros da sql, se houver, com a função db_param().
 *
 * @param db          conexão com banco de dados.
 */
int db_exec(DB *db);

/**
 * Obtém o valor de um campo no resultado do comando sql.
 *
 * @param  db   conexão com o banco de dados.
 * @param  nReg número do registro/linha no resultado do comando sql.
 * @param  nCol número da coluna, com base na ordem especificada pelo comando
 *              sql.
 * @return      valor do campo.
 */
const char *db_value(DB *db, int nReg, int nCol);

/**
 * Contexto passado pela função db_send(). Esta função é útil para obter o
 * contexto na callback que trata o resultado do comando sql.
 *
 * @param  db conexão com o banco de dados.
 * @return    contexto
 */
void *db_context(DB *db);

/**
 * Obtém a quantidade de registros retornado pelo comando sql.
 *
 * @param  db conexão com o banco de dados.
 * @return    quantidade de registros.
 */
int db_count(DB *db);

/**
 * Informa se existe algum erro no comando sql executado.
 *
 * @param  db conexão com o banco de dados.
 * @return   true, em caso de erro no comando sql, false, caso contrário.
 */
bool db_error(DB *db);

#endif
