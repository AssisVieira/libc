/*******************************************************************************
 *   Copyright 2020 Assis Vieira
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

/**
 * Buffer circular de tamanho pré-definido.
 */

#ifndef BUFF_H
#define BUFF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/uio.h>

/**
 * Número máximo de segmentos. Como a implentação é baseada em buffer circular,
 * haverá no máximo 2 segmentos.
 */
#define BUFF_SEGMENTS_MAX 2

typedef struct Buff Buff;

typedef struct BuffReader {
  Buff *buff;
} BuffReader;

typedef struct BuffWriter {
  Buff *buff;
} BuffWriter;

struct Buff {
  char *data;
  size_t iread;
  size_t iwrite;
  size_t imarkRead;
  size_t size;
  size_t used;
  size_t markUsed;
  struct iovec iov[BUFF_SEGMENTS_MAX];
  BuffReader reader;
  BuffWriter writer;
};

/**
 * Inicializa uma instância de buffer.
 *
 * Esta função deve ser chamada sempre antes de usar o buffer.
 *
 * @param  buff instância de buffer.
 * @param  size tamanho máximo do buffer.
 * @return      0, em caso de sucesso, -1, caso não há memória suficiente.
 */
int buff_init(Buff *buff, size_t size);

/**
 * Obtém um cursor de buffer para somente leitura.
 *
 * @param  buff buffer
 * @return      cursor de leitura.
 */
BuffReader *buff_reader(Buff *buff);

/**
 * Obtém um cursor de buffer para somente escrita.
 *
 * @param  buff buffer
 * @return      cursor de escrita.
 */
BuffWriter *buff_writer(Buff *buff);

/**
 * Limpa o buffer.
 *
 * Esta função é opcional e não libera a memória alocada, se desejar, consulte
 * buff_free().
 *
 * @param  buff instância de buffer.
 */
void buff_clear(Buff *buff);

/**
 * Libera toda a memória usada pelo buffer.
 *
 * Esta função deve sempre ser chamada quando o buffer for inicializado com a
 * função buff_init() e o buffer não for mais utilizado.
 *
 * Esta função não libera a memória usada pela estrutura Buff, esta função
 * libera apenas os recursos alocados pela função buff_init(). Instâncias de
 * Buff alocados dinâmicamente, por exemplo, com malloc(), devem ser liberados
 * também dinâmicamente, por exemplo, com free(), depois de chamar buff_free().
 *
 * @param buff instância de buffer.
 */
void buff_free(Buff *buff);

/**
 * Obtém a quantidade de bytes disponíveis para leitura.
 *
 * @param  buff instância de buffer.
 * @return      quantidade de bytes usados.
 */
size_t buff_used(const Buff *buff);

/**
 * Obtém a quantidade de bytes disponíveis para escrita.
 *
 * @param  buff instância de buffer.
 * @return      quantidade de bytes disponíveis.
 */
size_t buff_freespace(const Buff *buff);

/**
 * Escreve dados no final do buffer.
 *
 * @param  writer cursor de escrita.
 * @param  data dados a serem copiados para o buffer.
 * @param  size quantidade de bytes a serem copiados.
 * @return      quantidade de bytes que de fato foram copiados. Este número pode
 *              ser menor do que o valor espeficicado no parâmetro size, caso
 *              não haja espaço suficiente no buffer. Para descobrir o espaço
 *              disponível, consulte buff_freespace().
 */
int buff_writer_write(BuffWriter *writer, const char *data, size_t size);

/**
 * Escreve uma string formatada para o buffer.
 *
 * @param  writer cursor de escrita.
 * @param  fmt    string fortmatada.
 * @param  va     lista de argumentos.
 * @return        quantidade de bytes escritos, ou -1, em caso de espaço
 *                insuficiênte.
 */
ssize_t buff_writer_vprintf(BuffWriter *writer, const char *fmt, va_list va);
ssize_t buff_writer_printf(BuffWriter *writer, const char *fmt, ...);

/**
 * Obtém um segmento do buffer disponível para escrita.
 *
 * O tamanho do vetor pode ser obtido consultando buff_write_size().
 *
 * Após escrever dados no vetor retornado, a função buff_write_commit() deve ser
 * chamada para efetivar a escrita. Caso contrário, o conteúdo escrito será
 * perdido.
 *
 * @param  writer cursor de escrita.
 * @return      vetor livre para escrita.
 */
char *buff_writer_data(BuffWriter *writer);

/**
 * Obtém o tamanho do segmento retornado por buff_write_data().
 *
 * @param  writer cursor de escrita.
 * @return      quantidade de bytes disponíveis para escrita no vetor
 *              retornado por buff_write_data().
 */
size_t buff_writer_size(const BuffWriter *writer);

/**
 * Avança o cursor de escrita do buffer em n bytes.
 *
 * Caso o valor de nbytes seja maior que o número de bytes disponíveis para
 * escrita, veja buff_freespace(), o cursor é posicionado no final do buffer.
 * Nesse caso, a chamada para a função buff_isfull() deve retornar true.
 *
 * @param  buff   instância de buffer.
 * @param  nbytes quantidade de bytes a serem avançados.
 */
void buff_writer_commit(BuffWriter *writer, size_t nbytes);

bool buff_writer_isfull(const BuffWriter *writer);

/**
 * Lê os dados escritos no buffer e avança o cursor de leitura.
 *
 * Esta função não realiza nenhuma cópia. Ela apenas aponta o parâmetro data
 * para o início do vetor de leitura do buffer e retorna o tamanho do vetor.
 *
 * Observe que, para ler todo o conteúdo do buffer pode ser necessário
 * mais de uma chamada desta função, pois os dados podem estar organizados em
 * diferentes segmentos.
 *
 * Esta função pode ser vista como uma combinação das funções buff_read_data(),
 * buff_read_size() e buff_read_commit().
 *
 * O padrão idiomático para a leitura completa do buffer usando esta função é:
 *
 * size_t length;
 * const char *data;
 * size_t dataSize = 32;
 *
 * while ((length = buff_read(&buff, &data, dataSize)) > 0) {
 *    ...
 * }
 *
 * ou
 *
 * const char *data;
 * size_t dataSize = 32;
 *
 * while (!buff_isempty(buff)) {
 *    size_t length = buff_read(&buff, &data, dataSize);
 *    ...
 * }
 *
 * @param  reader cursor de leitura.
 * @param  data ponteiro que será inicializado para o início dos dados.
 * @param  maxlen quantidade máxima de bytes para leitura.
 * @return número de bytes que podem ser lidos do ponteiro data. Este número
 *         pode ser menor que o valor informado em maxlen, caso a leitura chegue
 *         ao fim.
 */
int buff_reader_read(BuffReader *reader, const char **data, size_t maxlen);

/**
 * Obtém um segmento do buffer disponível para leitura.
 *
 * O tamanho do segmento pode ser obtido consultando buff_read_size().
 *
 * Após ler o segmento, a função buff_read_commit() deve ser chamada para
 * efetivar a leitura. Caso contrário, o conteúdo lido nunca será consumido,
 * isto é, liberado do buffer para novos dados. Além disso, sem
 * buff_read_commit(), consecutivas chamadas para buff_read_data() sempre
 * retornará o mesmo segmento do buffer.
 *
 * @param  buff instância de buffer.
 * @return      vetor livre para escrita.
 */
const char *buff_reader_data(const BuffReader *reader);

/**
 * Marca a posição do cursor de leitura, caso queira desfazer commits, voltando
 * a posição inicial antes de realizar a leitura.
 *
 * @param  reader cursor de leitura.
 */
void buff_reader_mark(const BuffReader *reader);

/**
 * Volta o curso de leitura para a posição marcada pela última chamada da função
 * buff_reader_mark(). Caso buff_reader_mark() nunca tenha sido chamado,
 * a chamada para buff_reader_rewind() não modificará o cursor.
 *
 * @param  reader cursor de leitura.
 */
void buff_reader_rewind(const BuffReader *reader);

/**
 * Passa os segmentos de leitura para um vetor de buffers do tipo struct iovec.
 * Esta estrutura é utilizada pelas funções de sistema writev() e readv().
 *
 * @param reader cursor de leitura.
 * @param iovec  vetor de struct iovec.
 * @param count  quantidade de elementos no vetor iovec.
 * @param commit true, caso a leitura deva ser comitada, false, caso contrário.
 */
void buff_reader_iovec(BuffReader *reader, struct iovec **iovec, size_t *count,
                       bool commit);

/**
 * Obtém o tamanho do segmento retornado por buff_read_data().
 *
 * @param  buff instância de buffer.
 * @return      quantidade de bytes disponíveis para leitura no vetor
 *              retornado por buff_read_data().
 */
size_t buff_reader_size(const BuffReader *reader);

bool buff_reader_isempty(const BuffReader *reader);

/**
 * Avança o cursor de leitura do buffer em n bytes.
 *
 * Caso o valor de nbytes seja maior que o número de bytes disponíveis para
 * leitura, veja buff_used(), o cursor é posicionado no final do buffer. Nesse
 * caso, a chamada para a função buff_isempty() deve retornar true.
 *
 * @param  buff   instância de buffer.
 * @param  nbytes quantidade de bytes a serem avançados.
 */
void buff_reader_commit(BuffReader *reader, size_t nbytes);

/**
 * Verifica se o buffer esta vazio.
 *
 * @return true, se o buffer estiver vazio, false, caso contrário.
 */
bool buff_isempty(const Buff *buff);

/**
 * Verifica se o buffer esta cheio.
 *
 * @return true, se o buffer estiver cheio, false, caso contrário.
 */
bool buff_isfull(const Buff *buff);

#endif
