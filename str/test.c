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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

typedef struct test_t {
  size_t size;
  size_t length;
  char buff[];
} test_t;

static void testBasic() {
  // Assegura que o "char buff[]" não contribui para o tamanho da struct.
  assert(sizeof(test_t) == (sizeof(size_t) + sizeof(size_t)));

  // Assegura que buff pode ser avaliado como uma string clássica.
  const size_t size = 2;
  test_t *segment = malloc(sizeof(test_t) + size);
  segment->size = size;
  segment->length = 1;
  segment->buff[0] = 'a';
  segment->buff[1] = '\0';

  // Assegura que nem sempre a quantidade de caracteres corresponde
  // à quantidade de bytes alocados para representá-los.
  assert(2 + 1 /* \0 */ == sizeof("ã"));
  assert(1 + 1 /* \0 */ == sizeof("a"));

  // Assegura que strlen() não calcula a quantidade de caracteres,
  // mas sim a quantidade de bytes usados para representá-los,
  // ignorando o terminal nulo.
  assert(strlen("á") == 2);

  // Assegura que a arítmética com o ponteiro do tipo char *não* é inteligente
  // o suficiente para identificar caracteres que consomem 1 ou dois bytes.
  // Assim, não podemos inferir que p[1] se refere ao segundo caracterer da
  // string.
  char *p1 = "áb";
  assert(*(p1 + 1) != 'b');
  assert(p1[1] != 'b');
  char *p2 = "ab";
  assert(*(p2 + 1) == 'b');
  assert(p2[1] == 'b');

  free(segment);
}

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

  assert(str_casecmp(s1, s2) == 0);

  str_free(&s1);
  str_free(&s2);
}

/**
 * Assegura que str_fmt() permite adicionar uma string parametrizada ao final
 * de uma string str_t.
 */
static void testFmt() {
  {
    const char *arg1 = "abcdefg";
    const char *fmt = "%s";
    const char *strResp = "abcdefg";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const char *arg1 = "abcdefg";
    const char *arg2 = "hijklmnopq";
    const char *fmt = "%s%s";
    const char *strResp = "abcdefghijklmnopq";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const char *arg1 = "abcdefg";
    const char *arg2 = "hijklmnopq";
    const char *fmt = ".%s.%s.";
    const char *strResp = ".abcdefg.hijklmnopq.";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const char *arg1 = "abcdefg";
    const char *arg2 = "hijklmnopq";
    const char *fmt = "%s%%%s";
    const char *strResp = "abcdefg%hijklmnopq";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const char *arg1 = "abc";
    const char *arg2 = "def";
    const char *arg3 = "ghi";
    const char *arg4 = "jkl";
    const char *arg5 = "mno";
    const char *arg6 = "pqr";
    const char *arg7 = "stu";
    const char *arg8 = "vwx";
    const char *arg9 = "yz";
    const char *fmt = ".%s.%s.%s.%s.%s.%s.%s.%s.%s.";
    const char *strResp = ".abc.def.ghi.jkl.mno.pqr.stu.vwx.yz.";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded =
        str_fmt(&str, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  // Int
  {
    const int arg1 = 1234567890;
    const char *fmt = "%d";
    const char *strResp = "1234567890";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 1234567890;
    const int arg2 = 987654321;
    const char *fmt = "%d%d";
    const char *strResp = "1234567890987654321";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 1234567890;
    const int arg2 = 987654321;
    const char *fmt = ".%d.%d.";
    const char *strResp = ".1234567890.987654321.";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 1234567890;
    const int arg2 = 987654321;
    const char *fmt = "%d%%%d";
    const char *strResp = "1234567890%987654321";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1, arg2);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 100;
    const int arg2 = 200;
    const int arg3 = 300;
    const int arg4 = 400;
    const int arg5 = 500;
    const int arg6 = 600;
    const int arg7 = 700;
    const int arg8 = 800;
    const int arg9 = 900;
    const char *fmt = ".%d.%d.%d.%d.%d.%d.%d.%d.%d.";
    const char *strResp = ".100.200.300.400.500.600.700.800.900.";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded =
        str_fmt(&str, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 0;
    const char *fmt = "%d";
    const char *strResp = "0";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 10;
    const char *fmt = "%d";
    const char *strResp = "10";
    //---
    const size_t strRespLen = strlen(strResp);
    str_t *str = str_new(strRespLen + 1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1);
    assert(bytesAdded == strRespLen);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
  {
    const int arg1 = 1000;
    const char *fmt = "%d";
    const char *strResp = "1000";
    //---
    str_t *str = str_new(1);
    size_t bytesAdded = str_fmt(&str, fmt, arg1);
    assert(bytesAdded == 4);
    assert(str_len(str) == 4);
    assert(str_cmpcstr(str, strResp) == 0);
    str_free(&str);
  }
}

/**
 * Assegura a capacidade de um caracter ser adicionado no final de uma string
 * str_t.
 */
static void testAddC() {
  str_t *str = str_new(2);
  int res = str_addc(&str, 'A');
  assert(res == 0);
  assert(str_cstr(str)[0] == 'A');
  assert(str_cstr(str)[1] == '\0');
  str_free(&str);
}

/**
 * Assegura a capacidade de uma string convencional ser adicionada no final
 * de uma string str_t.
 */
static void testAddCStr() {
  str_t *str = str_new(6 + 1);

  int res = str_addcstr(&str, "Fulano");

  assert(res == 0);

  assert(str_len(str) == 6);

  assert(str_size(str) == 6 + 1);

  assert(str_cstr(str)[0] == 'F');
  assert(str_cstr(str)[1] == 'u');
  assert(str_cstr(str)[2] == 'l');
  assert(str_cstr(str)[3] == 'a');
  assert(str_cstr(str)[4] == 'n');
  assert(str_cstr(str)[5] == 'o');
  assert(str_cstr(str)[6] == '\0');

  str_free(&str);
}

/**
 * Assegura a capacidade de uma string str_t ser adicionada no final de uma
 * string str_t.
 */
static void testAdd() {
  str_t *str = str_clonecstr("Nome: ");
  str_t *str2 = str_clonecstr("Fulano");

  int res = str_add(&str, str2);

  assert(res == 0);

  assert(str_len(str) == 12);
  assert(str_size(str) == 12 + 1);

  const char *strFinal = str_cstr(str);
  assert(strFinal[0] == 'N');
  assert(strFinal[1] == 'o');
  assert(strFinal[2] == 'm');
  assert(strFinal[3] == 'e');
  assert(strFinal[4] == ':');
  assert(strFinal[5] == ' ');
  assert(strFinal[6] == 'F');
  assert(strFinal[7] == 'u');
  assert(strFinal[8] == 'l');
  assert(strFinal[9] == 'a');
  assert(strFinal[10] == 'n');
  assert(strFinal[11] == 'o');
  assert(strFinal[12] == '\0');

  str_free(&str);
  str_free(&str2);
}

/**
 * Assegura que uma instância de str_t pode ser criada a partir de uma
 * string tradicional e que a memória ocupada pela string de str_t deve ser o
 * mesmo da string tradicional.
 */
static void testNew2() {
  const char *name = "João Cabral de Melo Neto da Silva Bragança";
  const size_t nameLen = strlen(name);
  str_t *str = str_clonecstr(name);
  assert(str != NULL);
  assert(str_len(str) == nameLen);
  assert(str_size(str) == nameLen + 1 /* terminal null */);
  str_free(&str);
}

/**
 * Assegura que uma string vazia pode ser criada e que a capacidade mínima da
 * string é 1 byte, para sempre garantir espaço para o terminal nulo. Isso
 * também garante consistência com a função str_cstr(), que ao ser chamada nesse
 * contexto, deverá retornar uma string vazia, apenas com o terminal nulo, em
 * vez de retornar NULL.
 */
static void testNew() {
  str_t *str1 = str_new(1);

  assert(str1 != NULL);
  assert(str_size(str1) == 1);
  assert(str_len(str1) == 0);
  assert(str_cstr(str1)[0] == '\0');

  str_free(&str1);

  assert(str_new(0) == NULL);
}

static void testLength() {
  const char *name = "João Cabral de Melo Neto da Silva Bragança";
  const size_t nameLen = 44;
  str_t *str = str_clonecstr(name);

  assert(str_len(str) == nameLen);

  str_free(&str);
}

/**
 * Assegura que a string str_t pode ser expandida manualmente.
 */
static void testExpand() {
  str_t *str = str_new(10);

  str_addcstr(&str, "abc");

  assert(str_size(str) == 10);
  assert(str_len(str) == 3);
  assert(str_cmpcstr(str, "abc") == 0);

  str = str_expand(str, 10);

  assert(str != NULL);
  assert(str_len(str) == 3);
  assert(str_size(str) == 14);
  assert(str_cmpcstr(str, "abc") == 0);

  str_free(&str);
}

static void testRm() {
  {
    str_t *str = str_clonecstr("abcdef");

    assert(str_rm(str, 0, 2) == 0);
    assert(str_len(str) == 4);
    assert(str_size(str) == 7);
    assert(str_cmpcstr(str, "cdef") == 0);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("");

    assert(str_rm(str, 0, 1) == -1);
    assert(str_len(str) == 0);
    assert(str_size(str) == 1);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("");

    assert(str_rm(str, 0, 0) == -1);
    assert(str_len(str) == 0);
    assert(str_size(str) == 1);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("");

    assert(str_rm(str, 1, 0) == -1);
    assert(str_len(str) == 0);
    assert(str_size(str) == 1);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("abc");

    assert(str_rm(str, 0, 1) == 0);
    assert(str_len(str) == 2);
    assert(str_size(str) == 4);
    assert(str_cmpcstr(str, "bc") == 0);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("abc");

    assert(str_rm(str, 2, 1) == 0);
    assert(str_len(str) == 2);
    assert(str_size(str) == 4);
    assert(str_cmpcstr(str, "ab") == 0);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("abc");

    assert(str_rm(str, 1, 1) == 0);
    assert(str_len(str) == 2);
    assert(str_size(str) == 4);
    assert(str_cmpcstr(str, "ac") == 0);

    str_free(&str);
  }
  {
    str_t *str = str_clonecstr("abc");

    assert(str_rm(str, 0, 3) == 0);
    assert(str_len(str) == 0);
    assert(str_size(str) == 4);

    str_free(&str);
  }
}

int main() {
  testBasic();
  testStrCaseCmp();
  testNew();
  testNew2();
  testExpand();
  testAddC();
  testAddCStr();
  testAdd();
  testLength();
  testFmt();
  testRm();
  return 0;
}
