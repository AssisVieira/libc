str: módulo de manipulação de string dinâmica
=============================================

Exemplo de uso:

```c
#include "str.h"
#include <stdio.h>

int main() {
  str_t *nome = str_new(10);

  if (str_addcstr(&nome, "Fulano Beltrano")) {
    perror("Erro: str_addcstr().\n");
    return -1;
  }

  str_t *outroNome = str_clonecstr("João Silva");

  if (outroNome == NULL) {
    perror("Erro: str_clonecstr()\n");
    return -1;
  }

  if (str_cmp(nome, outroNome) == 0) {
    printf("'%s' é '%s'\n", str_cstr(nome), str_cstr(outroNome));
  } else {
    printf("'%s' não é '%s'\n", str_cstr(nome), str_cstr(outroNome));
  }

  str_free(&nome);
  str_free(&outroNome);

  return 0;
}
```



Índice
------

### Estrutura
  - [Estrutura da string](#estrutura-da-string)

### Criação
  - [Criar string vazia](#criar-string-vazia)
  - [Clonar string](#clonar-string)
  - [Clonar string convencional](#clonar-string-convencional)
  - [Clonar string convencional espeficificando o tamanho](#clonar-string-convencional-espeficificando-o-tamanho)

### Memória
  - [Liberar memória](#liberar-memória)
  - [Limpar string](#limpar-string)
  - [Expandir capacidade](#expandir-capacidade)
  - [Quantidade de bytes alocados](#quantidade-de-bytes-alocados)
  - [Quantidade de bytes consumidos](#quantidade-de-bytes-consumidos)

### String convencional
  - [Obter string convencional](#obter-string-convencional)
  - [Concatenar string convencional](#concatenar-string-convencional)
  - [Comparar string convencional](#comparar-string-convencional)

### UTF-8
  - [Quantidade de símbolos UTF-8](#quantidade-de-símbolos-utf-8)

### Concatenação
  - [Concatenar](#concatenação)
  - [Concatenar caracter](#concatenar-caracter)
  - [Concatenar string convencional](#concatenar-string-convencional)
  - [Concatenar string convencional espeficificando o tamanho](#concatenar-string-convencional-espeficificando-o-tamanho)

### Comparação
  - [Comparar](#comparação)
  - [Comparar string convencional](#comparar-string-convencional)
  - [Comparar ignorando caixa alta](#comparar-ignorando-caixa-alta)
  -
### Formatação
  - [Formatar string](#formatar-string)

---

## Estrutura da string

```c
typedef struct str_t str_t;
```

Cada string tem 16 bytes de cabeçalho + n bytes para representação de
caracteres + 1 byte de terminador nulo. Assim, cada string deve ter no
mínimo 17 bytes.



Criar string vazia
------------------

```c
str_t *str_new(size_t size);
```

##### Parâmetros
  - **size**: quantidade de bytes pre-alocados para armazenar a string,
  incluindo o terminal nulo.

##### Retorno
  - nova instância de str_t;



Clonar string convencional
--------------------------

```c
str_t *str_clonecstr(const char *cstr);
```

##### Parâmetros  
  - **cstr**: string convencional.

##### Retorno
  - uma instancia de *str_t*;



Clonar string convencional espeficificando o tamanho
----------------------------------------------------

```c
str_t *str_clonecstrlen(const char *cstr, size_t len);
```

##### Parâmetros  
  - **cstr**: string convencional.
  - **len**: quantidade de bytes a serem copiados da string convencional,
  ignorando o terminal nulo.

##### Retorno
  - uma instancia de *str_t*;



Liberar memória
---------------

```c
void str_free(str_t **str);
```

Libera a memória utilizada para manter a string e aponta a string para NULL.

##### Parâmetros

  - **str**: string a ser destruida.

##### Retorno

  - Sem retorno.



Obter string convencional
-------------------------

```c
const char *str_cstr(const str_t *str);
```

Retorna uma string convencional para apenas leitura.

##### Parâmetros

 - **str**: string str_t.

##### Retorno
  - vetor de char com terminador nulo (const char *).



Expandir Capacidade
-------------------

```c
str_t *str_expand(str_t *str, size_t needed);
```

Espande a capacidade de uma string, caso for necessário. Se a string já tiver o
espaço disponível, exigido pelo parâmetro *needed*, a string permacenerá
intacta, sem nenhum tipo de alocação.

##### Parâmetros
  - **str**: string a ser expandida.
  - **needed**: número de bytes disponíveis que a string deve conter.

##### Retorno
  - string inalterada, se a string fornecida já tiver os bytes
disponíveis; ou
  - nova string realocada com os bytes disponíveis; ou
  - NULL, em caso de problemas na realocação da nova string.



Concatenar
----------

```c
int str_add(str_t **dest, const str_t *orig);
```

Adiciona uma string str_t no final de outra string str_t.

##### Parâmetros
 - **dest**: string destino.
 - **orig**: string origem.

##### Retorno
 - 0, se a adição foi realizada com sucesso; ou -1, caso contrário.



Concatenar caracter
-------------------

```c
int str_addc(str_t **dest, char c);
```

Adiciona um caractere (char) no final de uma string str_t.

##### Parâmetros
  - **dest**: string destino.
  - **c**: caractere a ser adicionado.

##### Retorno
  - 0, se a adição foi realizada com sucesso, -1, caso contrário.



Concatenar string convencional
------------------------------

```c
int str_addcstr(str_t **dest, const char *orig);
```

Adiciona uma string convencional no final de string str_t.

##### Parâmetros
 - **dest**: string destino.
 - **orig**: string origem.

##### Retorno
 - 0, se a adição foi realizada com sucesso, -1, caso contrário.



Concatenar string convencional espeficificando o tamanho
---------------------------------------------------------

```c
int str_addcstrlen(str_t **dest, const char *orig, size_t len);
```

Adiciona uma string convencional no final de string str_t.

##### Parâmetros
  - **dest**: string destino.
  - **orig**: string origem.
  - **len**: quantidade de bytes a serem copiados da string de origem,
  ignorando o terminal nulo.

##### Retorno
  - 0, se a adição foi realizada com sucesso, -1, caso contrário.



Quantidade de símbolos UTF-8
----------------------------

```c
size_t str_utf8len(const str_t *str);
```

Calcula a quantidade de caracteres/símbolos com base na codificação UTF-8.

##### Parâmetros
  - **str**: string codificada em UTF-8.

##### Retorno
  - Quantidade de caracteres/símbolos.



Quantidade de bytes alocados
----------------------------

```c
size_t str_size(const str_t *str);
```

Calcula a quantidade de bytes alocados pela string, incluindo o terminal nulo.
A string pode ou não consumir todo o espaço alocado. Para conhecer apenas o
espaço consumido pela string, consulte str_len().

##### Parâmetros
  - **str**: string para calcular o espaço alocado.

##### Retorno
  - Quantidade de bytes.



Quantidade de bytes consumidos
------------------------------

```c
size_t str_len(const str_t *str);
```

Calcula a quantidade de bytes usados para representar a string, ignorando o
terminal nulo, tal como a função strlen() da biblioteca padrão.

##### Parâmetros
  - **str**: string para calcular o espaço consumido.

##### Retorno
  - Quantidade de bytes.



Limpar string
-------------

```c
str_t *str_clear(str_t *str);
```

Limpa a string, tornando-a vazia. Essa função não libera a memória consumida
pela string. O espaço alocado pela string permanece o mesmo.

##### Parâmetros
  - **str**: string a ser limpa.

##### Retorno
  - A mesma instância da string fornecida como parâmetro.



Comparar
--------

```c
int str_cmp(const str_t *s1, const str_t *s2);
```

Compara duas string str_t, considerando letras maisculas e minúsculas como
letras distintas.

##### Parâmetros
  - **s1**: string 1 a ser comparada.
  - **s2**: string 2 a ser comparada.

##### Retorno
  - 0, se s1 é igual a s2; -1, se s1 antecede s2; 1, se s2 antecede s1. Sempre
  se baseando na ordem dos símbolos codificados pela tabela ASCII.



Comparar string convencional
----------------------------

```c
int str_cmpcstr(const str_t *s1, const char *s2);
```

Compara uma string str_t e uma string convencional, considerando letras
maisculas e minúsculas como letras distintas.

##### Parâmetros
  - **s1**: string 1, do tipo str_t, a ser comparada.
  - **s2**: string 2, convencional, a ser comparada.

##### Retorno
  - 0, se s1 é igual a s2; -1, se s1 antecede s2; 1, se s2 antecede s1. Sempre
  se baseando na ordem dos símbolos codificados pela tabela ASCII.



Comparar ignorando caixa alta
-----------------------------

```c
int str_casecmp(const str_t *s1, const str_t *s2);
```

Compara duas string str_t, ignorando a distinção entre letras maiúsculas e
minúsculas.

##### Parâmetros
  - **s1**: string 1 a ser comparada.
  - **s2**: string 2 a ser comparada.

##### Retorno
  - 0, se s1 é igual a s2; -1, se s1 antecede s2; 1, se s2 antecede s1.
  Sempre se baseando na ordem dos símbolos codificados pela tabela ASCII.



Clonar string
-------------

```c
str_t *str_clone(const str_t *str);
```

Cria uma string copiando todo o conteúdo de outra string.

##### Parâmetros
  - **str**: string a ser clonada.

##### Retorno
  - instância da nova string, ou NULL, em caso de problemas na alocação da nova
  string.



Formatar string
---------------

```c
size_t str_fmt(str_t *str, const char *fmt, ...);
size_t str_fmtv(str_t *str, const char *fmt, va_list va);
```

Adiciona uma string formatada, a fmt, no final de outra string, a str. Esta
função não realiza alocação dinâmica, tal como as funções str_add(), str_addc()
e str_adds(). Se não houver espaço suficiente na string str, a a formatação
será encerrada.

##### Parâmetros
  - **str**: string a ser concatenada.
  - **fmt**: string formatada.
  - **VARARGS**: argumentos variadic para a string formatada.

##### Retorno
  - Quantidade de bytes adicionados na string ou -1 se a string não tiver mais
  espaço disponível.
