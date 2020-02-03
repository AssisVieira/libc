#include "hashTable.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void testCount();
static void testValue();
static void testContains();
static void testFree();
static void testSet();

int main() {
  testCount();
  testValue();
  testContains();
  testFree();
  testSet();

  return 0;
}

static void testSet() {
  HashTable *hash = hashTable_new(10);

  assert(hashTable_set(hash, "Nome", "Fulano") == 0);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testFree() {
  HashTable *hash = hashTable_new(10);

  hashTable_free(&hash);

  assert(hash == NULL);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testValue() {
  HashTable *hash = hashTable_new(10);

  hashTable_set(hash, "Nome", "Fulano");

  assert(strcmp(hashTable_value(hash, "Nome"), "Fulano") == 0);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testContains() {
  HashTable *hash = hashTable_new(10);

  hashTable_set(hash, "Nome", "Fulano");

  assert(hashTable_contains(hash, "Nome") == true);

  assert(hashTable_contains(hash, "Fulano") == false);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testCount() {
  HashTable *hash = hashTable_new(10);

  hashTable_set(hash, "Nome", "Fulano");
  hashTable_set(hash, "Idade", "30");
  hashTable_set(hash, "Altura", "1.70m");

  assert(hashTable_count(hash) == 3);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}
