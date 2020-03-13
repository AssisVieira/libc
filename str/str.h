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

#ifndef STR_H
#define STR_H

#include <stdarg.h>
#include <stdlib.h>

/**
 * Cada string tem 16 bytes de cabeçalho + n bytes para representação de
 * caracteres + 1 byte de terminador nulo. Assim, cada string deve ter no
 * mínimo 17 bytes.
 */
typedef struct str_t str_t;

/**
 * Cria uma string vazia.
 *
 * @param  size quantidade de bytes pre-alocados para armazenar a string,
 *              incluindo o terminal nulo.
 * @return      nova instância de str_t;
 */
str_t *str_new(size_t size);

/**
 * Cria uma string copiando de uma string convencional.
 *
 * @param  s string convencional.
 * @return   uma instancia de str_t;
 */
str_t *str_clonecstr(const char *cstr);

/**
 * Cria uma string copiando, parcial ou totalmente, de uma string convencional.
 *
 * @param  s string convencional.
 * @return   uma instancia de str_t;
 */
str_t *str_clonecstrlen(const char *cstr, size_t len);

/**
 * Libera a memória utilizada para manter a string e aponta a string para NULL.
 *
 * @param str string a ser destruida.
 */
void str_free(str_t **str);

/**
 * Obtém uma string convencional para apenas leitura.
 *
 * @param  str string do tipo str_t.
 * @return     vetor de char com terminador nulo.
 */
const char *str_cstr(const str_t *str);

/**
 * Espande a capacidade de uma string, caso for necessário.
 *
 * @param  str string a ser expandida.
 * @param  needed  número de bytes disponíveis que a string deve conter.
 * @return         string inalterada, se a string fornecida já tiver os bytes
 *                 disponíveis; ou nova string realocada com os bytes
 *                 disponíveis; ou NULL, em caso de problemas na realocação da
 *                 nova string.
 */
str_t *str_expand(str_t *str, size_t needed);

/**
 * Adiciona uma string str_t no final de outra string str_t.
 *
 * @param  dest  string destino.
 * @param  orig  string origem.
 * @return       0, se a adição foi realizada com sucesso, -1, caso contrário.
 */
int str_add(str_t **dest, const str_t *orig);

/**
 * Adiciona um caractere (char) no final de uma string str_t.
 *
 * @param  dest  string destino.
 * @param  c     caractere a ser adicionado.
 * @return       0, se a adição foi realizada com sucesso, -1, caso contrário.
 */
int str_addc(str_t **dest, char c);

/**
 * Insere um caractere (char) em uma determinada posição.
 *
 * @param  dest  string destino.
 * @param  pos   posição dentro da string, a partir de 0.
 * @param  c     caractere a ser inserido.
 * @return       0, se a inserção foi realizada com sucesso, -1, caso contrário.
 */
int str_setc(str_t **dest, size_t pos, char c);

/**
 * Adiciona uma string convencional no final de string str_t.
 *
 * Considerando `dest` uma string do tipo `str_t *` e `orig` uma string do tipo
 * `char *`, a chamada de função str_adds(&dest, orig) terá o mesmo efeito que
 * `str_addslen(&dest, orig, strlen(orig))`;
 *
 * @param  dest  string destino.
 * @param  orig  string origem.
 * @param  len   quantidade de bytes da string de origem, ignorando o terminal
 *               nulo.
 * @return       0, se a adição foi realizada com sucesso, -1, caso contrário.
 */
int str_addcstr(str_t **dest, const char *orig);

/**
 * Adiciona, parcial ou totalmente, uma string convencional no final de
 * string str_t.
 *
 * @param  dest  string destino.
 * @param  orig  string origem.
 * @param  len   quantidade de bytes da string de origem que devem ser copiados,
 *               ignorando o terminal nulo.
 * @return       0, se a adição foi realizada com sucesso, -1, caso contrário.
 */
int str_addcstrlen(str_t **dest, const char *orig, size_t len);

/**
 * Calcula a quantidade de caracteres/símbolos com base na codificação UTF-8.
 *
 * @param  str string codificada em UTF-8.
 * @return     quantidade de caracteres/símbolos.
 */
size_t str_utf8len(const str_t *str);

/**
 * Calcula a quantidade de bytes alocados pela string, incluindo o terminal
 * nulo.
 *
 * A string pode ou não consumir todo o espaço alocado. Para conhecer o espaço
 * consumido pela string, consulte str_len().
 *
 * @param  str string do tipo str_t.
 * @return     quantidade de bytes.
 */
size_t str_size(const str_t *str);

/**
 * Calcula a quantidade de bytes usados para representar a string, ignorando
 * o terminal nulo, tal como a função strlen() da biblioteca padrão.
 *
 * @param  str string do tipo str_t.
 * @return     quantidade de bytes.
 */
size_t str_len(const str_t *str);

/**
 * Limpa a string, tornando-a vazia.
 *
 * Essa função não libera a memória consumida pela string. O espaço alocado pela
 * string permanece o mesmo.
 *
 * @param  str string a ser limpa.
 * @return     a string limpa.
 */
str_t *str_clear(str_t *str);

/**
 * Compara duas string str_t, considerando letras maisculas e minúsculas como
 * letras distintas.
 *
 * @param  s1 string 1 a ser comparada.
 * @param  s2 string 2 a ser comparada.
 * @return    0, se s1 é igual a s2; -1, se s1 antecede alfabeticamente s2; 1,
 *            se s2 antecede alfabeticamente s1.
 */
int str_cmp(const str_t *s1, const str_t *s2);

/**
 * Compara uma string str_t e uma string convencional, considerando letras
 * maisculas e minúsculas como letras distintas.
 *
 * @param  s1 string 1 str_1 a ser comparada.
 * @param  s2 string 2 convencional a ser comparada.
 * @return    0, se s1 é igual a s2; -1, se s1 antecede alfabeticamente s2; 1,
 *            se s2 antecede alfabeticamente s1.
 */
int str_cmpcstr(const str_t *s1, const char *s2);

/**
 * Compara duas string str_t, ignorando a distinção entre letras maiúsculas e
 * minúsculas.
 *
 * @param  s1 string 1 a ser comparada.
 * @param  s2 string 2 a ser comparada.
 * @return    0, se s1 é igual a s2; -1, se s1 antecede alfabeticamente s2; 1,
 *            se s2 antecede alfabeticamente s1.
 */
int str_casecmp(const str_t *s1, const str_t *s2);

/**
 * Compara uma string str_t e uma string convencional, ignorando letras
 * maisculas e minúsculas como letras distintas.
 *
 * @param  s1 string 1 str_1 a ser comparada.
 * @param  s2 string 2 convencional a ser comparada.
 * @return    0, se s1 é igual a s2; -1, se s1 antecede alfabeticamente s2; 1,
 *            se s2 antecede alfabeticamente s1.
 */
int str_casecmpcstr(const str_t *s1, const char *s2);

/**
 * Compara uma string str_t e uma string convencional, ignorando letras
 * maisculas e minúsculas como letras distintas.
 *
 * Esta função realiza a comparação dos primeiros `len` bytes.
 *
 * @param  s1 string 1 str_1 a ser comparada.
 * @param  s2 string 2 convencional a ser comparada.
 * @param  len quantidade de bytes a serem comparados.
 * @return    0, se s1 é igual a s2; -1, se s1 antecede alfabeticamente s2; 1,
 *            se s2 antecede alfabeticamente s1.
 */
int str_casecmpcstrlen(const str_t *s1, const char *s2, size_t len);

/**
 * Cria uma string copiando todo o conteúdo de outra string.
 *
 * @param  str string a ser copiada.
 * @return     instância da nova string, ou NULL, em caso de problemas na
 *             alocação da nova string.
 */
str_t *str_clone(const str_t *str);

/**
 * Adiciona uma string parametrizada no final de outra string.
 *
 * @param  str     string a ser concatenada.
 * @param  fmt     string formatada.
 * @param  VARARGS argumentos variadic para a string formatada.
 * @return         quantidade de bytes adicionados na string ou -1 se a string
 *                 não tiver mais espaço disponível.
 */
size_t str_fmt(str_t **str, const char *fmt, ...);
size_t str_fmtv(str_t **str, const char *fmt, va_list va);

/**
 * Obtem uma string nula, baseado no padrão Null Object.
 *
 * A string retornada é uma string vazia que não pode ser modificada. Todas as
 * chamadas desta função retornará sempre a mesma instância. Use essa função
 * como uma alternativa a constante `NULL`.
 *
 * As chamadas de funções `str_size(str_null())` ou `str_len(str_null())` sempre
 * retornaram 0.
 *
 * Diferente das outras strings, a string nula ocupa 16 bytes de memória,
 * 1 byte a menos que as demais. Pois a string nula não contém o terminal nulo.
 */
const str_t *str_null();

/**
 * Retorna a string apontada pelo parâmetro str e o inicializa com uma nova
 * string vazia, com capacidade de 1 byte.
 *
 * Esta é uma função utilitária que evita o overhead da cópia de strings
 * e a criação de variáveis temporárias pra mover a string.
 *
 * @param  str string a ser movida e inicializada com uma nova string vazia.
 * @return     string que pertencia ao parâmetro `str`.
 */
str_t *str_move(str_t **str);

/**
 * Remove uma parte da string.
 * 
 * @param  str    string a ser modificada.
 * @param  start  índice da primeira posição a ser removida, inclusive.
 * @param  count  quantidade de posições a serem removidas.
 * @return        0, em caso de sucesso, -1, em caso de erros.
 */
int str_rm(str_t *str, int start, int count);

#endif
