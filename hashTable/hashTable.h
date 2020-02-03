#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct HashTableNode {
  char *key;
  void *value;
  struct HashTableNode *next;
} HashTableNode;

typedef struct HashTable {
  HashTableNode **table;
  size_t module;
} HashTable;

typedef struct HashTableIt {
  size_t row;
  HashTableNode *node;
  HashTable *hashTable;
} HashTableIt;

/**
 * Inicializa uma tabela hash, onde a chave é do tipo string e o valor é um
 * objeto.
 *
 * @param  modulo altura da tabela hash;
 * @return instancia da tabela ou NULL, caso não há memória suficiente.
 */
int hashTable_init(HashTable *hashTable, size_t module);

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
 * Inicializa o iterador para percorrer as entradas da tabela.
 *
 * @param hashTable tabela hash a ser percorrida.
 * @param it        iterador para ser inicializado.
 */
void hashTable_it(HashTable *hashTable, HashTableIt *it);

/**
 * Obtém a chave da entrada atual do iterador.
 *
 * @param  it iterador.
 * @return    chave.
 */
const char *hashTable_itKey(HashTableIt *it);

/**
 * Obtém o valor da entrada atual do iterador.
 *
 * @param  it iterador
 * @return    valor
 */
void *hashTable_itValue(HashTableIt *it);

/**
 * Verifica se existe alguma entrada, se houver, avança para a próxima.
 *
 * Esta função deve sempre ser chamada antes de consultar a chave e o valor da
 * entrada atual do iterador.
 *
 * @param  it iterador
 * @return    true, caso existe uma entrada para ser consultada, false, caso
 * contrário.
 */
bool hashTable_itNext(HashTableIt *it);

/**
 * Destroi a tabela, porém mantém intacto os valores de cada chave.
 *
 * @param hashTable tabela.
 */
void hashTable_free(HashTable *hashTable);

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
