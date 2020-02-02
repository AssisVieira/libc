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

#include "ioevent/ioevent.h"

#define DB_SQL_PARAMS_MAX 32

typedef struct DB DB;

typedef void (*onDBCallback)(DB *db);

/**
 * Inicializa o pool de conexões.
 *
 * @param  min quantidade mínima de conexões ociosas.
 * @param  max quantidade máxima de conexões ativas.
 * @return     0, em caso de sucesso, -1, em caso de erro.
 */
int db_openPool(int min, int max);

/**
 * Encerra o pool de conexões.
 */
void db_closePool();

/**
 * Obtém uma conexão do pool de conexões.
 *
 * @return conexão com o banco de dados.
 */
DB *db_open();

/**
 * Devolve uma conexão para o pool de conexões.
 *
 * @param db conexão a ser devolvida.
 */
void db_close(DB *db);

/**
 * Configura o comando sql a ser enviado.
 *
 * @param db  conexão com o banco de dados.
 * @param sql comando sql.
 */
void db_sql(DB *db, const char *sql);

/**
 * Libera todos os recursos utilizados pelo comando sql.
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

/**
 * Envia o comando sql para o banco de dados. Esta função deve ser sempre
 * chamada após a configuração da sql, com a função db_sql() e após a
 * configurações dos parâmetros da sql, se houver, com a função db_param().
 *
 * @param db          conexão com banco de dados.
 * @param context     objeto a ser passado para a callback.
 * @param onCmdResult callback para tratar a resposta do comando sql.
 */
void db_send(DB *db, void *context, onDBCallback onCmdResult);

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
