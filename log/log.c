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

#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////

typedef struct Log {
  FILE *file;
  LogFilter filter;
  bool terminal;
} Log;

////////////////////////////////////////////////////////////////////////////////

/**
 * Inicializa a configuração do log:
 * - Sem escrita em disco;
 * - Sem filtro;
 * - Registro do log apenas na saída padrão (terminal).
 */
static Log LOG = {NULL, NULL, true};

////////////////////////////////////////////////////////////////////////////////

static void log_log(FILE *stdfile, const char *level, const char *module,
                    const char *fmt, va_list args);

////////////////////////////////////////////////////////////////////////////////

int log_open(const char *path) {
  if (path == NULL) {
    return -1;
  }

  LOG.file = fopen(path, "a+");

  if (LOG.file == NULL)
    return -1;

  log_info("log", "Arquivo aberto.\n");

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int log_close() {
  int r = 0;

  if (LOG.file != NULL) {
    log_info("log", "Arquivo fechado.\n");
    r = fclose(LOG.file);
  }

  if (r != 0)
    return -1;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void log_info(const char *module, const char *fmt, ...) {
  va_list(args);
  va_start(args, fmt);
  fprintf(stdout, "\e[97m");
  log_log(stdout, "INFO", module, fmt, args);
  fprintf(stdout, "\e[0m");
  va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void log_warn(const char *module, const char *fmt, ...) {
  va_list(args);
  va_start(args, fmt);
  fprintf(stdout, "\e[93m");
  log_log(stdout, "WARN", module, fmt, args);
  fprintf(stdout, "\e[0m");
  va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void log_dbug(const char *module, const char *fmt, ...) {
#ifdef DEBUG
  va_list(args);
  va_start(args, fmt);
  fprintf(stdout, "\e[35m");
  log_log(stdout, "DBUG", module, fmt, args);
  fprintf(stdout, "\e[0m");
  va_end(args);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void log_dbugbin(const char *module, const char *buff, size_t size,
                 const char *fmt, ...) {
#ifdef DEBUG
  if (LOG.filter != NULL && LOG.filter(module, fmt))
    return;

  va_list(args);
  va_start(args, fmt);

  if (LOG.terminal) {
    va_list args2;
    va_copy(args2, args);
    fprintf(stdout, "\e[35m");
    fprintf(stdout, "[%ld] [%-10.10s] [DBUG]: ", time(NULL), module);
    vfprintf(stdout, fmt, args2);
    for (size_t i = 0; i < size; i++) {
      fprintf(stdout, "%02X", buff[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "\e[0m");
    va_end(args2);
  }

  if (LOG.file != NULL) {
    fprintf(LOG.file, "[%ld] [%-10.10s] [DBUG]: ", time(NULL), module);
    vfprintf(LOG.file, fmt, args);
    for (size_t i = 0; i < size; i++) {
      fprintf(LOG.file, "%02X", buff[i]);
    }
    fprintf(LOG.file, "\n");
  }

  va_end(args);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void log_erro(const char *module, const char *fmt, ...) {
  va_list(args);
  va_start(args, fmt);
  fprintf(stdout, "\e[91m");
  log_log(stderr, "ERRO", module, fmt, args);
  fprintf(stdout, "\e[0m");
  va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void log_filter(LogFilter filter) { LOG.filter = filter; }

////////////////////////////////////////////////////////////////////////////////

void log_terminal(bool terminal) { LOG.terminal = terminal; }

////////////////////////////////////////////////////////////////////////////////

static void log_log(FILE *stdfile, const char *level, const char *module,
                    const char *fmt, va_list args) {
  if (LOG.filter != NULL && LOG.filter(module, fmt))
    return;

  if (LOG.terminal) {
    va_list args2;
    va_copy(args2, args);
    fprintf(stdfile, "[%ld] [%-10.10s] [%s]: ", time(NULL), module, level);
    vfprintf(stdfile, fmt, args2);
    va_end(args2);
  }

  if (LOG.file != NULL) {
    fprintf(LOG.file, "[%ld] [%-10.10s] [%s]: ", time(NULL), module, level);
    vfprintf(LOG.file, fmt, args);
  }
}
