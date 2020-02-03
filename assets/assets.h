#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

/**
 * Carrega para a memória todos os arquivos do diretório e subdiretórios
 * de um endereço.
 *
 * @param  path diretório raiz dos assets.
 * @return      0, em caso de sucesso, -1 em caso de erro.
 */
int assets_open(const char *path);

/**
 * Obtém o cursor de leitura do
 *
 * @param  path [description]
 * @return      [description]
 */
const char *assets_get(const char *path);

size_t assets_size(const char *path);

void assets_close();

#endif
