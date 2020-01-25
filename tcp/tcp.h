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
 
#ifndef TCP_H
#define TCP_H

#include "buff/buff.h"

typedef enum TcpStatus {
  TCP_OK,
  TCP_ERROR,
  TCP_DISCONNECTED,
  TCP_TRY_AGAIN
} TcpStatus;

int tcp_accept(int fd);

TcpStatus tcp_noblock(int fd);

int tcp_listen(const char *port, int backlog);

TcpStatus tcp_write(int fd, BuffReader *reader);

TcpStatus tcp_read(int fd, BuffWriter *writer);

TcpStatus tcp_close(int fd);

#endif
