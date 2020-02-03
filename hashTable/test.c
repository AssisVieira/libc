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
static void testIterator();

int main() {
  testCount();
  testValue();
  testContains();
  testFree();
  testSet();
  testIterator();

  return 0;
}

static void testIterator() {
  HashTable hash;

  hashTable_init(&hash, 10);

  hashTable_set(&hash, "nome", "fulano");
  hashTable_set(&hash, "idade", "30");
  hashTable_set(&hash, "altura", "1,70");

  HashTableIt it;

  hashTable_it(&hash, &it);

  assert(hashTable_itNext(&it) == true);
  assert(strlen(hashTable_itKey(&it)) > 0);
  assert(strlen(hashTable_itValue(&it)) > 0);

  assert(hashTable_itNext(&it) == true);
  assert(strlen(hashTable_itKey(&it)) > 0);
  assert(strlen(hashTable_itValue(&it)) > 0);

  assert(hashTable_itNext(&it) == true);
  assert(strlen(hashTable_itKey(&it)) > 0);
  assert(strlen(hashTable_itValue(&it)) > 0);

  assert(hashTable_itNext(&it) == false);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testSet() {
  HashTable hash;

  hashTable_init(&hash, 10);

  assert(hashTable_set(&hash, "Nome", "Fulano") == 0);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testFree() {
  HashTable hash;

  hashTable_init(&hash, 10);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testValue() {
  HashTable hash;

  hashTable_init(&hash, 10);

  hashTable_set(&hash, "Nome", "Fulano");

  assert(strcmp(hashTable_value(&hash, "Nome"), "Fulano") == 0);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testContains() {
  HashTable hash;

  hashTable_init(&hash, 10);

  hashTable_set(&hash, "Nome", "Fulano");

  assert(hashTable_contains(&hash, "Nome") == true);

  assert(hashTable_contains(&hash, "Fulano") == false);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}

static void testCount() {
  HashTable hash;

  hashTable_init(&hash, 10);

  hashTable_set(&hash, "Nome", "Fulano");
  hashTable_set(&hash, "Idade", "30");
  hashTable_set(&hash, "Altura", "1.70m");

  assert(hashTable_count(&hash) == 3);

  hashTable_free(&hash);

  printf("%s() is ok.\n", __FUNCTION__);
}
