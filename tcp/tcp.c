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

#include "tcp.h"

// Para usar o accept4 é preciso definir esta macro.
// Veja mais na manpage do accept4.
#define _GNU_SOURCE

#include "log/log.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////

TcpStatus tcp_read(int fd, BuffWriter *writer) {
  while (!buff_writer_isfull(writer)) {
    ssize_t r = read(fd, buff_writer_data(writer), buff_writer_size(writer));

    if (r < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        log_dbug("tcp", "(fd %d) >>> TENTAR DEPOIS\n", fd);
        return TCP_TRY_AGAIN;
      } else {
        log_dbug("tcp", "(fd %d) >>> ERRO : %d - %s\n", fd, errno,
                 strerror(errno));
        return TCP_ERROR;
      }
    }

    if (r == 0) {
      log_dbug("tcp", "(fd %d) >>> DESCONECTADO\n", fd);
      return TCP_DISCONNECTED;
    }

    log_dbugbin("tcp", buff_writer_data(writer), r, "(fd %d) >>> (%d) ", fd, r);

    buff_writer_commit(writer, r);
  }

  log_dbug("tcp", "(fd %d) >>> OK\n", fd);

  return TCP_OK;
}

////////////////////////////////////////////////////////////////////////////////

TcpStatus tcp_close(int fd) {
  log_dbug("tcp", "Fechando conexão %d\n", fd);
  shutdown(fd, SHUT_RDWR);
  if (close(fd) != 0)
    return TCP_ERROR;
  return TCP_OK;
}

////////////////////////////////////////////////////////////////////////////////

TcpStatus tcp_write(int fd, BuffReader *reader) {

  while (!buff_reader_isempty(reader)) {
    ssize_t nwritten =
        write(fd, buff_reader_data(reader), buff_reader_size(reader));

    if (nwritten < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        log_dbug("tcp", "(fd %d) <<< TENTE DEPOIS", fd);
        return TCP_TRY_AGAIN;
      } else {
        log_dbug("tcp", "(fd %d) <<< ERRO: %d - %s\n", fd, errno,
                 strerror(errno));
        return TCP_ERROR;
      }
    }

    log_dbugbin("tcp", buff_reader_data(reader), nwritten, "(fd %d) <<< (%d) ",
                fd, nwritten);

    buff_reader_commit(reader, nwritten);
  }

  log_dbug("tcp", "(fd %d) <<< OK\n", fd);

  return TCP_OK;
}

////////////////////////////////////////////////////////////////////////////////

int tcp_listen(const char *port, int backlog) {
  int sockOuvinte = -1;
  struct addrinfo hints, *listaAddr, *addrPtr;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // Permite IPv4 ou IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP Socket
  hints.ai_flags = AI_PASSIVE;     // Aceita qualquer IP

  int ret = getaddrinfo(NULL, port, &hints, &listaAddr);
  if (ret != 0)
    return -1;

  for (addrPtr = listaAddr; addrPtr != NULL; addrPtr = addrPtr->ai_next) {
    sockOuvinte =
        socket(addrPtr->ai_family, addrPtr->ai_socktype, addrPtr->ai_protocol);

    if (sockOuvinte == -1)
      continue;

    int optval = 1;

    if (setsockopt(sockOuvinte, SOL_SOCKET, SO_REUSEPORT, &optval,
                   sizeof(optval)) != 0) {
      close(sockOuvinte);
      return -1;
    }

    if (bind(sockOuvinte, addrPtr->ai_addr, addrPtr->ai_addrlen) == 0)
      break;

    close(sockOuvinte);
  }

  if (addrPtr == NULL) {
    close(sockOuvinte);
    return -1;
  }

  freeaddrinfo(listaAddr);

  if (tcp_noblock(sockOuvinte) != 0) {
    close(sockOuvinte);
    return -1;
  }

  if (listen(sockOuvinte, backlog) != 0) {
    close(sockOuvinte);
    return -1;
  }

  return sockOuvinte;
}

////////////////////////////////////////////////////////////////////////////////

TcpStatus tcp_noblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);

  if (flags == -1)
    return TCP_ERROR;

  int s = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  if (s == -1)
    return TCP_ERROR;

  return TCP_OK;
}

////////////////////////////////////////////////////////////////////////////////

int tcp_accept(int ouvinte) {
  struct sockaddr clienteEndereco;
  socklen_t clienteEnderecoTamanho = sizeof(clienteEndereco);
  int fd = accept4(ouvinte, (struct sockaddr *)&clienteEndereco,
                   &clienteEnderecoTamanho, SOCK_NONBLOCK);

  if (fd == -1)
    return -1;

  return fd;
}
