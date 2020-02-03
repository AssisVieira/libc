#include "hashTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct HashTableNode {
  char *key;
  void *value;
  struct HashTableNode *next;
} HashTableNode;

struct HashTable {
  HashTableNode **table;
  size_t module;
};

////////////////////////////////////////////////////////////////////////////////

static size_t hashTable_hash(const char *str, size_t module);
static HashTableNode *hashTable_nodeSearch(HashTableNode *node,
                                           const char *key);
static HashTableNode *hashTable_nodeNew(const char *key, void *value,
                                        HashTableNode *next);
static void hashTable_nodeFree(HashTableNode **node);
static size_t hashTable_nodeCount(const HashTableNode *node);

////////////////////////////////////////////////////////////////////////////////

HashTable *hashTable_new(size_t module) {
  HashTable *hashTable = malloc(sizeof(HashTable));

  if (hashTable == NULL) {
    return NULL;
  }

  hashTable->module = module;
  hashTable->table = malloc(sizeof(HashTableNode *) * module);

  if (hashTable->table == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < module; i++) {
    hashTable->table[i] = NULL;
  }

  return hashTable;
}

////////////////////////////////////////////////////////////////////////////////

void hashTable_free(HashTable **hashTable) {
  for (size_t i = 0; i < (*hashTable)->module; i++) {
    hashTable_nodeFree(&(*hashTable)->table[i]);
  }

  free((*hashTable)->table);

  free(*hashTable);

  *hashTable = NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool hashTable_contains(const HashTable *hashTable, const char *key) {
  size_t hash = hashTable_hash(key, hashTable->module);

  HashTableNode *node = hashTable->table[hash];

  if (hashTable_nodeSearch(node, key) != NULL) {
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

int hashTable_set(HashTable *hashTable, const char *key, void *value) {
  size_t hash = hashTable_hash(key, hashTable->module);

  HashTableNode *nodeHead = hashTable->table[hash];

  HashTableNode *node = hashTable_nodeSearch(nodeHead, key);

  if (node != NULL && strcmp(node->key, key) == 0) {
    node->value = value;
    return 0;
  }

  hashTable->table[hash] = hashTable_nodeNew(key, value, nodeHead);

  if (hashTable->table[hash] == NULL) {
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void *hashTable_value(const HashTable *hashTable, const char *key) {
  size_t hash = hashTable_hash(key, hashTable->module);

  HashTableNode *node = hashTable_nodeSearch(hashTable->table[hash], key);

  if (node == NULL) {
    return NULL;
  }

  return node->value;
}

////////////////////////////////////////////////////////////////////////////////

size_t hashTable_count(const HashTable *hashTable) {
  size_t count = 0;
  for (int i = 0; i < hashTable->module; i++) {
    count += hashTable_nodeCount(hashTable->table[i]);
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////

static size_t hashTable_nodeCount(const HashTableNode *node) {
  size_t count = 0;
  while (node != NULL) {
    count++;
    node = node->next;
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////

static HashTableNode *hashTable_nodeSearch(HashTableNode *node,
                                           const char *key) {
  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return node;
    }
    node = node->next;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static HashTableNode *hashTable_nodeNew(const char *key, void *value,
                                        HashTableNode *next) {
  HashTableNode *node = malloc(sizeof(HashTableNode));
  if (node != NULL) {
    node->key = (key != NULL) ? strdup(key) : NULL;
    node->value = value;
    node->next = next;
  }
  return node;
}

////////////////////////////////////////////////////////////////////////////////

static void hashTable_nodeFree(HashTableNode **node) {
  HashTableNode *p = *node;
  HashTableNode *next = NULL;
  while (p != NULL) {
    next = p->next;
    free(p->key);
    free(p);
    p = next;
  }
  *node = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static size_t hashTable_hash(const char *str, size_t module) {
  int hash = 7;

  while (*str) {
    hash = hash * 31 + *str++;
  }

  return hash % module;
}
