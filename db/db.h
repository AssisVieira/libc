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

typedef struct DBPool DBPool;

/**
 * Função que deve tratar o resultado do comando sql enviado com a função
 * db_send().
 *
 * @param db      conexão utilizada pra executar o comando solicitado.
 * @param context contexto fornecido ao chamar a função db_send().
 * @param error   true, se houve algum erro com o comando, false, caso
 *                contrário. Esta flag deve sempre ser verificada antes de
 *                utilizar a conexão. Pois se houver erro, a conexão será
 *                descartada quando a callback retornar e qualquer operação com
 *                a conexão será nula.
 */
typedef void (*onDBCallback)(DB *db, void *context, bool error);

/**
 * Cria um o pool de conexões.
 *
 * @param  strConn  string de conexão.
 * @param  async    true, se as conexões devem ser assíncronas, false, caso
 *                  contrário.
 * @param  maxIdle  número máximo de conexões ociosas.
 * @param  maxBusy  número máximo de conexões ocupadas.
 * @return          pool de conexões, em caso de sucesso, NULL, em caso de erro.
 */
DBPool *db_pool_create(const char *strConn, bool async, size_t maxIdle,
                       size_t maxBusy);

/**
 * Destroi o pool de conexões, fechando todas as conexões ativas e liberando
 * toda a memória utilizada pelo pool.
 */
void db_pool_destroy(DBPool *dbpool);

/**
 * Tenta obter uma conexão ociosa do pool. Caso não exista conexões ociosas, uma
 * nova conexão é criada se e somente se o número de conexões ocupadas for menor
 * que o número máximo especificado na criação do pool. Veja mais em
 * db_pool_create(). Caso contrário, a função ficará bloqueada até que uma
 * conexão seja retornada ao pool, usando db_close().
 *
 * @return conexão com o banco de dados.
 */
DB *db_pool_get(DBPool *dbpool);

/**
 * Enables the pool.
 */
void db_pool_enable(DBPool *pool);

/**
 * Disables the pool. In this case, connections from pool will be destroyed
 * when closed, they must not return to the pool.
 */
void db_pool_disable(DBPool *pool);

/**
 * Cria uma conexão com um banco de dados.
 *
 * @param  strConn  string de conexão.
 * @param  async    true, se a conexão deve ser assíncrona, false, caso
 * contrário.
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
 * Encerra o uso da conexão e limpa os dados usados até o momento. Se não houver
 * nenhum erro na transação, então efetiva tudo o que ocorreu no bloco da
 * transação. Se a conexão foi obtida de um pool, logo a conexão será
 * mantida aberta, sendo apenas devolvida ao pool. A conexão será destruída
 * apenas se uma das seguintes setenças for verdadeira:
 *     1. A conexão não pertence a um pool.
 *     2. A conexão pertence a um pool e...
 *        a) A conexão está marcada com erro.
 *        b) O pool estiver desativado.
 *        c) O número de conexões ociosas chegou ao limite máximo.
 * @param db conexão a ser encerrada.
 * @return 0, em caso de sucesso, -1, em caso de erro na transação. Por isso, 
 * 	       o retorno da chamada deste método nunca deve ser ignorado. Esse 
 * 	       conselho também vale para transação composta apenas  por um único
 * 	       comando select, pois erros na obtenção de dados usando db_value*() 
 * 	       só são verificados por db_error() ou db_close().
 */
int db_close(DB *db);

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
void db_paramDouble(DB *db, double value);

/**
 * Executa um comando sql de forma assíncrona. Esta função deve ser sempre
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
double db_valueDouble(DB *db, int nReg, int nCol);
int db_valueInt(DB *db, int nReg, int nCol);

/**
 * Obtém a quantidade de registros retornado pelo comando sql.
 *
 * @param  db conexão com o banco de dados.
 * @return    quantidade de registros.
 */
size_t db_count(DB *db);

/**
 * Informa se existe algum erro no comando sql executado.
 *
 * @param  db conexão com o banco de dados.
 * @return   true, em caso de erro no comando sql, false, caso contrário.
 */
bool db_error(DB *db);

/**
 * Obtém o número de conexões do pool que estão ocupadas.
 * Isto é, a quantidade de conexões que foram obtidas com db_pool_get() e ainda
 * não foram devolvidas com db_close(). O número de conexões ocupadas não deve
 * ser maior que a capacidade máxima fornecida no momento da construção do pool,
 * com a função db_pool_create().
 */
int db_pool_num_busy(const DBPool *pool);

/**
 * Obtém o número de conexões ociosas no pool.
 */
int db_pool_num_idle(const DBPool *pool);

#endif
