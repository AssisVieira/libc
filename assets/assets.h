#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

/**
 * Carrega para a memória todos os arquivos do diretório e subdiretórios
 * de um endereço.
 *
 * @param  path endereço relativo do diretório raiz.
 * @return      0, em caso de sucesso, -1 em caso de erro.
 */
int assets_open(const char *path);

/**
 * Obtém o conteúdo de um arquivo.
 *
 * @param  path endereço relativo do arquivo.
 * @return      conteúdo do arquivo.
 */
const char *assets_get(const char *path);

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
