#ifndef ASSETS_H
#define ASSETS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Carrega para a memória todos os arquivos e subdiretórios
 * de um diretório.
 *
 * @param  path endereço relativo do diretório raiz.
 * @return      0, em caso de sucesso, -1 em caso de erro.
 */
int assets_open(const char *dir);

/**
 * Obtém o conteúdo de um arquivo.
 *
 * @param  path endereço relativo do arquivo.
 * @return      conteúdo do arquivo.
 */
const char *assets_get(const char *path);

/**
 * Verifica se path é um diretório dentro da hierarquia de diretórios
 * carregados pela função assets_open().
 *
 * @return true, caso path é um diretório, false, caso contrário.
 */
bool assets_isDir(const char *path);

/**
 * Verifica se um path existe dentro da hierarquia de arquivos e diretórios
 * carregados pela função assets_open().
 *
 * @return true, caso o path exista, false, caso contrário.
 */
bool assets_exists(const char *path);

/**
 * Obtém o tamanho de um arquivo.
 *
 * @param path endereço relativo do arquivo.
 * @return     quantidade de bytes do arquivo.
 */
size_t assets_size(const char *path);

/**
 * Fecha o módulo assets, liberando toda a memória utilizada pelo módulo,
 * incluive o conteúdo dos arquivos carregados.
 */
void assets_close();

#endif
