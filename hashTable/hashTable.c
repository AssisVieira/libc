#include "hashTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

static size_t hashTable_hash(const char *str, size_t module);
static HashTableNode *hashTable_nodeSearch(HashTableNode *node,
                                           const char *key);
static void hashTable_nodeFree(HashTableNode **node);

////////////////////////////////////////////////////////////////////////////////

int hashTable_init(HashTable *hashTable, size_t module) {
  if (hashTable == NULL) {
    goto error;
  }

  hashTable->module = module;
  hashTable->table = malloc(sizeof(HashTableNode *) * module);

  if (hashTable->table == NULL) {
    goto error;
  }

  for (size_t i = 0; i < module; i++) {
    hashTable->table[i] = NULL;
  }

  return 0;

error:
  hashTable_free(hashTable);
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

void hashTable_free(HashTable *hashTable) {
  for (size_t i = 0; i < hashTable->module; i++) {
    hashTable_nodeFree(&hashTable->table[i]);
  }

  free(hashTable->table);
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

  HashTableNode *headNode = hashTable->table[hash];

  HashTableNode *oldNode = hashTable_nodeSearch(headNode, key);

  if (oldNode != NULL && strcmp(oldNode->key, key) == 0) {
    oldNode->value = value;
    return 0;
  }

  HashTableNode *newNode = malloc(sizeof(HashTableNode));

  if (newNode == NULL) {
    return -1;
  }

  newNode->key = (key != NULL) ? strdup(key) : NULL;
  newNode->value = value;
  newNode->next = hashTable->table[hash];

  hashTable->table[hash] = newNode;

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
  HashTableNode *node = NULL;
  size_t count = 0;
  for (int i = 0; i < hashTable->module; i++) {
    node = hashTable->table[i];
    while (node != NULL) {
      ++count;
      node = node->next;
    }
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////

void hashTable_it(HashTable *hashTable, HashTableIt *it) {
  it->hashTable = hashTable;
  it->node = NULL;
  it->row = -1;
}

const char *hashTable_itKey(HashTableIt *it) { return it->node->key; }

////////////////////////////////////////////////////////////////////////////////

void *hashTable_itValue(HashTableIt *it) { return it->node->value; }

////////////////////////////////////////////////////////////////////////////////

bool hashTable_itNext(HashTableIt *it) {
  if (it->node == NULL || it->node->next == NULL) {
    for (it->row = it->row + 1; it->row < it->hashTable->module; it->row++) {
      it->node = it->hashTable->table[it->row];
      if (it->node != NULL) {
        return true;
      }
    }
    return false;
  }

  if (it->node->next != NULL) {
    it->node = it->node->next;
    return true;
  }

  return false;
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
