
Módulo de log
=============

Registra logs informativos, de atenção, de erros ou de debug do sistema. Este
módulo representa uma única instância para todo o sistema.

Caso de uso 1: registrando as mensagens de log apenas no console.
--------------

```c
#include "log/log.h"
#include <stdio.h>

int main() {
    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    return 0;
}
```

Caso de uso 2: registrando as mensagens de log no disco e no console.
--------------

```c
#include "log/log.h"
#include <stdio.h>

int main() {
    if (log_open("log.txt")) {
        perror("Erro na abertura do log.\n");
        return -1;
    }

    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    if (log_close()) {
        perror("Erro ao encerrar o log.");
        return -1;
    }

    return 0;
}
```

Caso de uso 3: registrando as mensagens de log apenas no disco.
--------------

```c
#include "log/log.h"
#include <stdio.h>

int main() {
    if (log_open("log.txt")) {
        perror("Erro na abertura do log.\n");
        return -1;
    }

    log_terminal(false);

    log_info("meu-modulo", "minha %s.\n", "mensagem informativa");

    log_warn("meu-modulo", "minha %s.\n", "mensagem de atenção");

    log_dbug("meu-modulo", "minha %s.\n", "mensagem de debug");

    log_erro("meu-modulo", "minha %s.\n", "mensagem de erro");

    log_terminal(true);

    if (log_close()) {
        perror("Erro ao encerrar o log.");
        return -1;
    }

    return 0;
}
```

Referência
----------

#### Protótipo de filtro de log

```c
typedef bool (*LogFilter)(const char *module, const char *fmt);
```

Protótipo da função que deve ser implementada pelo filtro de log.

Veja mais nos comentários da função log_filter().

###### Parâmetros
  - module: nome do módulo. A mesma string passada quando as funções log_info, log_debug ou log_error são chamadas.

  - fmt: string de formatação do log. A mesma string passada quando as funções log_info, log_debug ou log_error são chamadas.

###### Retorno
  - true, se o log deve ser filtrado, false, caso contrário.

--------------------------------------------------------------------------------

#### Abrir arquivo de log

```c
int log_open(const char *path);
```

Abre o arquivo de log.

Após chamar esta função, todos os logs serão registrados em arquivo no disco.

A chamada desta função é opcional. Se a função não for chamada, os logsnão*
serão registrados no disco. Se a função for invocada, logo, a função
log_close() também deverá ser invocada a fim de fechar o arquivo aberto por
esta função.

Preferencialmente, se o registro do log do sistema em arquivo for um
requisito, a sugestão é para que as funções log_open() e log_close() sejam
invocadas no início e no fim do método main(), respectivamente.

###### Parâmetros
  - path: endereço do arquivo do log.

###### Retorno
  - 0, em caso de sucesso, -1 em caso de erro na abertura do arquivo para escrita.

--------------------------------------------------------------------------------

#### Fecha o arquivo de log

```c
int log_close();
```

Fecha o arquivo de log aberto pela função log_open();

Esta função deve sempre ser chamada apenas quando a função log_open() for
invocada.

###### Retorno
  - 0, em caso de fechamento com sucesso, -1, em caso de problemas no fechamento, por exemplo, espaço insuficiente no disco.

--------------------------------------------------------------------------------

#### Log Informativo

```c
void log_info(const char *module, const char *fmt, ...);
```

Registra log informativo sobre o sistema.

A formatação da mensagem segue a mesma especificação da função fprintf().

###### Parâmetros
- module: nome do módulo que está originando o registro do log.
- fmt: string de formatação da mensagem do log.
- VARARGS: argumentos a serem usados na formatação da mensagem do log.

--------------------------------------------------------------------------------

#### Log de Atenção

```c
void log_warn(const char *module, const char *fmt, ...);
```

Registra log de atenção sobre o sistema.

A formatação da mensagem segue a mesma especificação da função fprintf().

###### Parâmetros
  - module: nome do módulo que está originando o registro do log.
  - fmt: string de formatação da mensagem do log.
  - VARARGS: argumentos a serem usados na formatação da mensagem do log.

--------------------------------------------------------------------------------

#### Log de Depuração

```c
void log_dbug(const char *module, const char *fmt, ...);
```

Registra log de depuração sobre o sistema.

A formatação da mensagem segue a mesma especificação da função fprintf().

Para habilitar essa função é preciso fornecer a macro DEBUG nos parâmetros
da compilação. Caso contrário, os registros de debug serão ignorados.

###### Parâmetros
- module: nome do módulo que está originando o registro do log.
- fmt: string de formatação da mensagem do log.
- VARARGS: argumentos a serem usados na formatação da mensagem do log.

--------------------------------------------------------------------------------

#### Log de Depuração para Buffer

```c
void log_dbugbin(const char *module, unsigned char *buff, size_t size, const char *fmt, ...);
```

Registra log de depuração sobre o sistema.

A formatação da mensagem segue a mesma especificação da função fprintf().

Esta função realiza o log de buffs.

###### Parâmetros
  - module: nome do módulo que está originando o registro do log.
  - buff: vetor representando o buff.
  - size: quantidade de bytes do buff.
  - fmt: string de formatação da mensagem do log.
  - VARARGS: argumentos a serem usados na formatação da mensagem do log.

--------------------------------------------------------------------------------

#### Log de Erros

```c
void log_erro(const char *module, const char *fmt, ...);
```

Registra log de erro sobre o sistema.

A formatação da mensagem segue a mesma especificação da função fprintf().

###### Parâmetros
  - module: nome do módulo que está originando o registro do log.
  - fmt: string de formatação da mensagem do log.
  - VARARGS: argumentos a serem usados na formatação da mensagem do log.

--------------------------------------------------------------------------------

#### Filtro de Log

```c
void log_filter(LogFilter filter);
```

Filtra as mensagens de log, independente do canal de saída, seja disco
ou console.

Esta é uma função opcional. O comportamente padrão é que nenhum log seja
filtrado.

###### Parâmetros

  - filter: Uma função que será invocada a cada registro de log. A função deve receber duas strings como argumento: o nome do módulo e a string de formatação do log. Se o log deve ser ignorado, isto é, filtrado, logo a função deverá retorna true, caso contrário, deverá retornar false. Se desejar remover qualquer filtro configurado anteriormente, apenas informe NULL.

--------------------------------------------------------------------------------

#### Saida de Log para Console

```c
void log_terminal(bool terminal);
```

Determina se o log deve ser exibido nas saídas padrão do console: stdout e
stderr.

Esta função é opcional. O comportamente padrão é que todos os logs sejam
exibidos no console.

###### Parâmetros
  - terminal: true, caso o logs devam ser exibidos no console, false, caso contrário.
