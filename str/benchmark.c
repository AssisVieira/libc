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

#include "str.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

static void testStrCaseCmp() {
  const size_t size = 256;
  char lowerCase[size];
  char upperCase[size];

  for (int i = 0; i < size; i++) {
    lowerCase[i] = tolower(i);
    upperCase[i] = toupper(i);
  }

  str_t *s1 = str_clonecstrlen(lowerCase, size);
  str_t *s2 = str_clonecstrlen(upperCase, size);

  const int ntests = 1000 * 1000;

  {
    clock_t begin = clock();
    for (int i = 0; i < ntests; i++) {
      assert(str_casecmp(s1, s2) == 0);
    }
    clock_t end = clock();
    double timeElapsed = ((double)(end - begin) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (str_casecmp): %f ms.\n", timeElapsed);
  }

  {
    const char *s11 = str_cstr(s1);
    const char *s22 = str_cstr(s2);
    clock_t begin1 = clock();
    for (int i = 0; i < ntests; i++) {
      assert(strcasecmp(s11, s22) == 0);
    }
    clock_t end1 = clock();
    double timeElapsed1 = ((double)(end1 - begin1) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (strcasecmp): %f ms.\n", timeElapsed1);
  }

  str_free(&s1);
  str_free(&s2);
}

static void testFmt() {
  char *name = "Fulano Beltrano";
  int yearsOld = 30;
  int height = 170;
  long long time = 123456789101112;
  const char *fmt = "%% Olá, Brasil! Meu nome é %s, tenho %d anos e minha "
                    "altura é %d cm: %l %%.";
  const char *stdFmt = "%% Olá, Brasil! Meu nome é %s, tenho %d anos e minha "
                       "altura é %d cm: %ld %%.";
  const char *strFmted = "% Olá, Brasil! Meu nome é Fulano Beltrano, tenho "
                         "30 anos e minha altura é 170 cm: 123456789101112 %.";
  str_t *str = str_new(strlen(strFmted) + 1);
  const int ntests = 1000 * 1000;

  {
    clock_t begin = clock();
    for (int i = 0; i < ntests; i++) {
      str_clear(str);
      size_t bytesAdded = str_fmt(str, fmt, name, yearsOld, height, time);
      assert(bytesAdded > 0);
      assert(str_cmpcstr(str, strFmted) == 0);
    }
    clock_t end = clock();
    double timeElapsedMs = ((double)(end - begin) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (str_fmt): %f\n", timeElapsedMs);
  }

  {
    char str1[128];
    clock_t begin1 = clock();
    for (int i = 0; i < ntests; i++) {
      str1[0] = '\0';
      size_t bytesAdded = sprintf(str1, stdFmt, name, yearsOld, height, time);
      assert(bytesAdded > 0);
      assert(strcmp(str1, strFmted) == 0);
    }
    clock_t end1 = clock();
    double timeElapsedMs1 = ((double)(end1 - begin1) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (sprintf): %f\n", timeElapsedMs1);
  }

  str_free(&str);
}

static void testLength() {
  const char *name = "João Cabral de Melo Neto da Silva Bragança";
  const size_t nameLen = strlen(name);
  const int ntests = 1000 * 1000;

  {
    str_t *str = str_clonecstrlen(name, nameLen);
    clock_t begin = clock();
    for (int i = 0; i < ntests; i++) {
      assert(str_len(str) == nameLen);
    }
    clock_t end = clock();
    double timeElapsedMs = ((double)(end - begin) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (str_len): %f\n", timeElapsedMs);
    str_free(&str);
  }

  {
    clock_t begin1 = clock();
    for (int i = 0; i < ntests; i++) {
      assert(strlen(name) == nameLen);
    }
    clock_t end1 = clock();
    double timeElapsedMs1 = ((double)(end1 - begin1) / CLOCKS_PER_SEC) * 1000;
    printf("Time elapsed (strlen): %f\n", timeElapsedMs1);
  }
}

int main() {
  testStrCaseCmp();
  testLength();
  testFmt();
  return 0;
}
