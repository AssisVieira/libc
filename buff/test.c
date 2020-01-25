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

#include "buff.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void testWrite();
static void testRead();
static void testManyReadAndWrite();
static void testWriterPrintf();

int main() {
  testWrite();
  testRead();
  testManyReadAndWrite();
  testWriterPrintf();
  return 0;
}

static void testWriterPrintf() {
  Buff buff;

  buff_init(&buff, 1000);

  BuffWriter *writer = buff_writer(&buff);

  size_t nwritten = buff_writer_printf(
      writer,
      "String: %s; Int: %d; Long: %l; LongLong: %ll; IntHex: %x; IntHexUpper: "
      "%X; LongHex: %lx; LongLongHex: %llx;",
      "Jhon", 20, (long)30, (long long)40, 50, 60, (long)70, (long long)80);

  assert(nwritten > 0);

  assert(buff_isfull(&buff) == false);

  assert(buff_isempty(&buff) == false);

  assert(buff_freespace(&buff) > 0);

  char result[] = "String: Jhon; Int: 20; Long: 30; LongLong: 40; IntHex: 32; "
                  "IntHexUpper: 3C; LongHex: 46; LongLongHex: 50;";

  BuffReader *reader = buff_reader(&buff);

  assert(buff_reader_size(reader) == strlen(result));
  assert(memcmp(buff_reader_data(reader), result, strlen(result)) == 0);

  buff_free(&buff);
}

static void testWrite() {
  Buff buff;

  buff_init(&buff, 10);

  BuffWriter *writer = buff_writer(&buff);

  size_t nwritten = buff_writer_write(writer, "abc", 3);

  assert(nwritten == 3);

  assert(buff_isfull(&buff) == false);

  assert(buff_isempty(&buff) == false);

  assert(buff_freespace(&buff) == 7);

  buff_free(&buff);
}

static void testRead() {
  Buff buff;
  const char *c;

  buff_init(&buff, 3);

  BuffWriter *writer = buff_writer(&buff);
  BuffReader *reader = buff_reader(&buff);

  buff_writer_write(writer, "abc", 3);

  size_t nread0 = buff_reader_read(reader, &c, 1);

  assert(nread0 == 1);
  assert(c[0] == 'a');
  assert(buff_used(&buff) == 2);
  assert(buff_freespace(&buff) == 1);

  size_t nread1 = buff_reader_read(reader, &c, 1);

  assert(nread1 == 1);
  assert(c[0] == 'b');
  assert(buff_used(&buff) == 1);
  assert(buff_freespace(&buff) == 2);

  size_t nread2 = buff_reader_read(reader, &c, 1);

  assert(nread2 == 1);
  assert(c[0] == 'c');
  assert(buff_used(&buff) == 0);
  assert(buff_freespace(&buff) == 3);

  size_t nread3 = buff_reader_read(reader, &c, 1);

  assert(nread3 == 0);
  assert(buff_used(&buff) == 0);
  assert(buff_freespace(&buff) == 3);

  assert(buff_isempty(&buff) == true);

  buff_free(&buff);
}

static void testManyReadAndWrite() {
  Buff buff;
  const char *data;
  size_t nread;
  size_t nwritten;

  buff_init(&buff, 10);

  BuffWriter *writer = buff_writer(&buff);
  BuffReader *reader = buff_reader(&buff);

  nwritten = buff_writer_write(writer, "abc", 3);
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 3);
  assert(data[0] == 'a');
  assert(data[1] == 'b');
  assert(data[2] == 'c');

  nwritten = buff_writer_write(writer, "abc", 3);
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 3);
  assert(data[0] == 'a');
  assert(data[1] == 'b');
  assert(data[2] == 'c');

  nwritten = buff_writer_write(writer, "abc", 3);
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 3);
  assert(data[0] == 'a');
  assert(data[1] == 'b');
  assert(data[2] == 'c');

  nwritten = buff_writer_write(writer, "abc", 3);
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 1);
  assert(data[0] == 'a');
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 2);
  assert(data[0] == 'b');
  assert(data[1] == 'c');

  nwritten = buff_writer_write(writer, "abc", 3);
  nread = buff_reader_read(reader, &data, 3);
  assert(nwritten == 3);
  assert(nread == 3);
  assert(data[0] == 'a');
  assert(data[1] == 'b');
  assert(data[2] == 'c');

  buff_free(&buff);
}
