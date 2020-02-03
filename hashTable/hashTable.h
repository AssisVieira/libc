#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct HashTable HashTable;

/**
 * Cria uma tabela hash, onde a chave é do tipo string e o valor é um objeto.
 *
 * @param  modulo altura da tabela hash;
 * @return instancia da tabela ou NULL, caso não há memória suficiente.
 */
HashTable *hashTable_new(size_t module);

/**
 * Insere o valor de uma chave.
 *
 * @param  hashTable  tabela hash.
 * @param  key        chave.
 * @param  value      valor ou NULL.
 * @return            0, se criado com sucesso, -1, se houver algum erro.
 */
int hashTable_set(HashTable *hashTable, const char *key, void *value);

/**
 * Destroi a tabela, porém mantém intacto os valores de cada chave.
 *
 * @param hashTable tabela.
 */
void hashTable_free(HashTable **hashTable);

/**
 * Calcula e retorna a quantidade de entradas na tabela.
 *
 * @param  hashTable tabela.
 * @return     quantidade de entradas.
 */
size_t hashTable_count(const HashTable *hashTable);

/**
 * Retorna o valor de determinada chave.
 *
 * @param  hashTable   tabela hash.
 * @param  key         chave.
 * @return             valor da chave.
 */
void *hashTable_value(const HashTable *hashTable, const char *key);

/**
 * Verifica se na tabela hash contém uma determinada chave, independente
 * do valor configurado, NULL ou qualquer outro objeto.
 *
 * @param  hashTable   tabela hash.
 * @param  key         chave
 * @return             1, caso a chave exista, 0, caso contrário.
 */
bool hashTable_contains(const HashTable *hashTable, const char *key);

#endif
