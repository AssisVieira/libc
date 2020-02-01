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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static size_t itoa(long long value, int radix, bool uppercase, bool unsig,
                   char *buff, size_t zero_pad);

////////////////////////////////////////////////////////////////////////////////

int buff_init(Buff *buff, size_t size) {
  buff->data = malloc(size);
  buff->iread = 0;
  buff->imarkRead = 0;
  buff->iwrite = 0;
  buff->size = size;
  buff->used = 0;
  buff->markUsed = 0;
  buff->reader.buff = buff;
  buff->writer.buff = buff;

  if (buff->data == NULL) {
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void buff_reader_iovec(BuffReader *reader, struct iovec **iovec, size_t *count,
                       bool commit) {
  *count = 0;

  if (!commit) {
    buff_reader_mark(reader);
  }

  while (!buff_reader_isempty(reader)) {
    reader->buff->iov[*count].iov_base = (void *)buff_reader_data(reader);
    reader->buff->iov[*count].iov_len = buff_reader_size(reader);
    buff_reader_commit(reader, reader->buff->iov[*count].iov_len);
    *count = *count + 1;
  }

  if (!commit) {
    buff_reader_rewind(reader);
  }

  *iovec = reader->buff->iov;
}

void buff_reader_mark(const BuffReader *reader) {
  reader->buff->imarkRead = reader->buff->iread;
  reader->buff->markUsed = reader->buff->used;
}

void buff_reader_rewind(const BuffReader *reader) {
  reader->buff->iread = reader->buff->imarkRead;
  reader->buff->used = reader->buff->markUsed;
}

////////////////////////////////////////////////////////////////////////////////

BuffReader *buff_reader(Buff *buff) { return &buff->reader; }

////////////////////////////////////////////////////////////////////////////////

BuffWriter *buff_writer(Buff *buff) { return &buff->writer; }

////////////////////////////////////////////////////////////////////////////////

void buff_clear(Buff *buff) {
  buff->iread = 0;
  buff->iwrite = 0;
  buff->used = 0;
}

////////////////////////////////////////////////////////////////////////////////

void buff_free(Buff *buff) {
  free(buff->data);
  buff->iread = 0;
  buff->iwrite = 0;
  buff->size = 0;
  buff->used = 0;
  buff->data = NULL;
}

////////////////////////////////////////////////////////////////////////////////

size_t buff_used(const Buff *buff) { return buff->used; }

////////////////////////////////////////////////////////////////////////////////

size_t buff_freespace(const Buff *buff) { return buff->size - buff->used; }

////////////////////////////////////////////////////////////////////////////////

int buff_writer_write(BuffWriter *writer, const char *data, size_t size) {
  size_t i = 0;

  while (i < size && !buff_writer_isfull(writer)) {
    size_t segmentSize = buff_writer_size(writer);
    size_t nwrite = ((size - i) < segmentSize) ? (size - i) : segmentSize;
    memcpy(buff_writer_data(writer), data + i, nwrite);
    buff_writer_commit(writer, nwrite);
    i += nwrite;
  }

  return i;
}

////////////////////////////////////////////////////////////////////////////////

char *buff_writer_data(BuffWriter *writer) {
  return writer->buff->data + writer->buff->iwrite;
}

////////////////////////////////////////////////////////////////////////////////

size_t buff_writer_size(const BuffWriter *writer) {
  return (writer->buff->iread <= writer->buff->iwrite &&
          writer->buff->used < writer->buff->size)
             ? writer->buff->size - writer->buff->iwrite
             : writer->buff->iread - writer->buff->iwrite;
}

////////////////////////////////////////////////////////////////////////////////

void buff_writer_commit(BuffWriter *writer, size_t nbytes) {
  nbytes = (nbytes > buff_freespace(writer->buff))
               ? buff_freespace(writer->buff)
               : nbytes;
  writer->buff->iwrite = (writer->buff->iwrite + nbytes) % writer->buff->size;
  writer->buff->used = writer->buff->used + nbytes;
}

////////////////////////////////////////////////////////////////////////////////

bool buff_writer_isfull(const BuffWriter *writer) {
  return buff_isfull(writer->buff);
}

////////////////////////////////////////////////////////////////////////////////

int buff_reader_read(BuffReader *reader, const char **data, size_t len) {
  *data = buff_reader_data(reader);

  size_t segmentSize = buff_reader_size(reader);

  size_t nread = (len < segmentSize) ? len : segmentSize;

  buff_reader_commit(reader, nread);

  return nread;
}

////////////////////////////////////////////////////////////////////////////////

const char *buff_reader_data(const BuffReader *reader) {
  return reader->buff->data + reader->buff->iread;
}

////////////////////////////////////////////////////////////////////////////////

size_t buff_reader_size(const BuffReader *reader) {
  return (reader->buff->iread < reader->buff->iwrite || reader->buff->used == 0)
             ? reader->buff->iwrite - reader->buff->iread
             : reader->buff->size - reader->buff->iread;
}

////////////////////////////////////////////////////////////////////////////////

void buff_reader_commit(BuffReader *reader, size_t nbytes) {
  nbytes = (nbytes > reader->buff->used) ? reader->buff->used : nbytes;
  reader->buff->iread = (reader->buff->iread + nbytes) % reader->buff->size;
  reader->buff->used = reader->buff->used - nbytes;
}

////////////////////////////////////////////////////////////////////////////////

bool buff_reader_isempty(const BuffReader *reader) {
  return buff_isempty(reader->buff);
}

////////////////////////////////////////////////////////////////////////////////

bool buff_isempty(const Buff *buff) { return (buff->used == 0) ? true : false; }

////////////////////////////////////////////////////////////////////////////////

bool buff_isfull(const Buff *buff) {
  return (buff_freespace(buff) == 0) ? true : false;
}

////////////////////////////////////////////////////////////////////////////////

ssize_t buff_writer_printf(BuffWriter *writer, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  ssize_t r = buff_writer_vprintf(writer, fmt, va);
  va_end(va);
  return r;
}

ssize_t buff_writer_vprintf(BuffWriter *writer, const char *fmt, va_list va) {
  size_t written = 0;

  while (!buff_writer_isfull(writer) && *fmt) {
    size_t maxLen = buff_writer_size(writer);
    size_t len = 0;

    for (; len < maxLen && *fmt; fmt++) {
      if (*fmt != '%') {
        buff_writer_data(writer)[len++] = *fmt;
        continue;
      }

      fmt++;

      if (*fmt == 's') {
        const char *buff = va_arg(va, const char *);
        while (*buff && len < maxLen) {
          buff_writer_data(writer)[len++] = *buff++;
        }
        continue;
      }

      if (*fmt == 'd' || *fmt == 'l') {
        long long num;
        int radix = 10;
        bool uppercase = false;
        char next = (*fmt == '\0') ? '\0' : fmt[1];

        if (*fmt == 'd') {
          num = va_arg(va, int);
        } else {
          if (next == 'l') {
            fmt++;
            next = (*fmt == '\0') ? '\0' : fmt[1];
            num = va_arg(va, long long);
          } else {
            num = va_arg(va, long);
          }
        }

        if (next == 'X' || next == 'x') {
          radix = 16;
          uppercase = (next == 'X') ? true : false;
          fmt++;
          next = (*fmt == '\0') ? '\0' : fmt[1];
        }

        char buff[32];
        char *buffP = buff;
        size_t buffLen = itoa(num, radix, uppercase, false, buff, 0);

        while (buffLen-- && len < maxLen) {
          buff_writer_data(writer)[len++] = *buffP++;
        }

        continue;
      }

      if (*fmt == 'X' || *fmt == 'x') {
        int num = va_arg(va, int);
        int radix = 16;
        bool uppercase = (*fmt == 'X') ? true : false;

        char buff[32];
        char *buffP = buff;
        size_t buffLen = itoa(num, radix, uppercase, false, buff, 0);

        while (buffLen-- && len < maxLen) {
          buff_writer_data(writer)[len++] = *buffP++;
        }

        continue;
      }

      buff_writer_data(writer)[len++] = *fmt;
    }

    buff_writer_commit(writer, len);

    written += len;
  }

  if (*fmt != '\0') {
    return -1;
  }

  return written;
}

////////////////////////////////////////////////////////////////////////////////

static size_t itoa(long long value, int radix, bool uppercase, bool unsig,
                   char *buff, size_t zero_pad) {
  const char digit1[] = "0123456789ABCDEF";
  const char digit2[] = "0123456789abcdf";
  const char *digit = (uppercase) ? digit1 : digit2;
  char *buffp = buff;
  int negative = 0;
  size_t i;
  size_t len;

  if (radix > 16)
    return 0;

  if (value < 0 && !unsig) {
    negative = 1;
    value = -value;
  }

  while (value > 0) {
    *buffp++ = digit[value % radix];
    value /= radix;
  };

  for (i = 0; i < zero_pad; i++)
    *buffp++ = '0';

  if (negative)
    *buffp++ = '-';

  len = (size_t)(buffp - buff);
  for (i = 0; i < len / 2; i++) {
    char j = buff[i];
    buff[i] = buff[len - i - 1];
    buff[len - i - 1] = j;
  }

  return len;
}
