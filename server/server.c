#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#include "buff/buff.h"
#include "io/io.h"
#include "log/log.h"
#include "vetor/vetor.h"

////////////////////////////////////////////////////////////////////////////////

static const int FLAG_TCP_NODELAY = 1;
static const int FLAG_REUSE_PORT = 1;

////////////////////////////////////////////////////////////////////////////////

typedef struct Server {
  int fd;
  bool canAccept;
  Vetor *clients;
  size_t numClients;
  ServerParams params;
  bool close;
} Server;

////////////////////////////////////////////////////////////////////////////////

typedef struct Client {
  int fd;
  Buff outbox;
  bool canWrite;
  Buff inbox;
  bool busy;
  bool canRead;
  bool readClosed;
} Client;

////////////////////////////////////////////////////////////////////////////////

static thread_local Server server;

////////////////////////////////////////////////////////////////////////////////

// static void server_freeClient(Client *client);
static Client *server_newClient();
static void server_read(Client *client);
static void server_write(Client *client);
static void server_onClientEvent(void *arg, int fd, IOEvent events);
static void server_acceptClients();
static void server_onListenEvent(void *arg, int fd, IOEvent events);
static void server_processInbox(Client *client);

////////////////////////////////////////////////////////////////////////////////

int server_start(ServerParams params) {
  server.params = params;
  server.fd = -1;
  server.canAccept = false;
  server.numClients = 0;
  server.clients = vetor_criar(params.maxClients);

  if (server.clients == NULL) goto error;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = inet_addr(params.host);
  address.sin_family = AF_INET;
  address.sin_port = htons(params.port);

  server.fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server.fd == -1) goto error;

  if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEPORT, &FLAG_REUSE_PORT,
                 sizeof(FLAG_REUSE_PORT))) {
    goto error;
  }

  if (bind(server.fd, &address, sizeof(address))) goto error;

  if (fcntl(server.fd, F_SETFL, O_NONBLOCK)) goto error;

  if (listen(server.fd, 10)) goto error;

  log_info("server", "Concorrência máxima: %d\n", params.maxClients);
  log_info("server", "Aguardando conexões na porta: %d.\n", params.port);

  if (io_add(server.fd, IO_READ | IO_EDGE_TRIGGERED, NULL,
             server_onListenEvent)) {
    goto error;
  }

  return io_run(10);

error:
  if (server.fd != -1) close(server.fd);
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

static void server_onListenEvent(void *arg, int fd, IOEvent events) {
  if (events & IO_READ) {
    server.canAccept = true;
    server_acceptClients();
    return;
  }

  if (events & IO_ERROR) {
    log_erro("server",
             "Unknown error on the listen socket. Stoping server...\n");
    server_stop(EXIT_FAILURE);
    return;
  }

  log_erro("server", "Unknown event on the listen socket (events = %x).\n",
           events);
}

////////////////////////////////////////////////////////////////////////////////

static void server_acceptClients() {
  while (server.canAccept && server.numClients < server.params.maxClients) {
    struct sockaddr address;
    socklen_t addressTamanho = sizeof(address);

    int clientFd = accept4(server.fd, &address, &addressTamanho, SOCK_NONBLOCK);

    if (clientFd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        server.canAccept = false;
        log_dbug("server", "Try again.\n");
        break;
      } else {
        log_erro("server", "tcp_accept(): %d - %s\n", errno, strerror(errno));
        continue;
      }
    }

    if (setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &FLAG_TCP_NODELAY,
                   sizeof(FLAG_TCP_NODELAY))) {
      log_erro("server", "client %d >>> setsockopt(TCP_NODELAY): %d - %s\n",
               clientFd, errno, strerror(errno));
      continue;
    }

    server.numClients++;

    log_dbug("server", "client %d >>> connection accepted [%d/%d].\n", clientFd,
             server.numClients, server.params.maxClients);

    Client *client = vetor_item(server.clients, clientFd);

    if (client == NULL) {
      client = server_newClient();

      if (client == NULL) {
        log_erro("server", "client %d >>> server_newClient() is NULL.\n",
                 clientFd);
        server_close(clientFd);
        break;
      }

      vetor_inserir(server.clients, clientFd, client);
    }

    client->fd = clientFd;
    client->canRead = false;
    client->readClosed = false;
    client->canWrite = true;
    client->busy = false;
    buff_clear(&client->inbox);
    buff_clear(&client->outbox);

    server.params.onConnected(client->fd);

    if (io_add(client->fd, IO_READ | IO_EDGE_TRIGGERED, NULL,
               server_onClientEvent)) {
      log_erro("server", "io_add(): %d - %s.\n", errno, strerror(errno));
      server_close(clientFd);
      continue;
    }
  }

  if (server.close) {
    log_info("server", "Close listen socket: %d.\n", server.fd);
    if (close(server.fd)) {
      log_erro("server", "close(): %d - %s.\n", errno, strerror(errno));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void server_send(int clientFd, const void *buff, size_t size) {
  Client *client = vetor_item(server.clients, clientFd);

  BuffWriter *writer = buff_writer(&client->outbox);

  if (buff_writer_write(writer, buff, size) != size) {
    log_erro("server", "buff_writer_write()\n");
    server_close(clientFd);
    return;
  }

  server_write(client);

  if (!client->busy) {
    server_processInbox(client);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void server_onClientEvent(void *arg, int fd, IOEvent events) {
  Client *client = vetor_item(server.clients, fd);

  if (events & IO_READ) {
    log_dbug("server", "client %d >>> can read.\n", client->fd);
    client->canRead = true;
    server_read(client);
    return;
  }

  if (events & IO_WRITE) {
    log_dbug("server", "client %d <<< can write.\n", client->fd);
    client->canWrite = true;
    server_write(client);
    return;
  }

  if (events & IO_CLOSED) {
    log_dbug("server", "client %d - closed.\n", client->fd);
    server_close(client->fd);
    return;
  }

  if (events & IO_ERROR) {
    log_erro("server", "client %d - unknown error. Closing client...\n",
             client->fd);
    server_close(client->fd);
    return;
  }

  log_erro("server", "client %d - unknown event (events = %x).\n", client->fd,
           events);
}

////////////////////////////////////////////////////////////////////////////////

static void server_write(Client *client) {
  struct iovec *iovec = NULL;
  size_t iovecCount = 0;

  if (!client->canWrite) {
    log_dbug("server",
             "client %d >>> outbox is busy, waiting for the write signal.\n",
             client->fd);
    return;
  }

  BuffReader *reader = buff_reader(&client->outbox);

  while (true) {
    buff_reader_iovec(reader, &iovec, &iovecCount, false);

    ssize_t nwritten = writev(client->fd, iovec, iovecCount);

    if (nwritten >= 0) {
      buff_reader_commit(reader, nwritten);
      log_dbug("server", "client %d <<< (%d bytes)\n", client->fd, nwritten);
      if (buff_reader_isempty(reader)) {
        client->busy = false;
        break;
      } else {
        continue;
      }
    }

    if (errno == EAGAIN) {
      log_dbug("server", "client %d <<< can't write, try again.\n", client->fd);
      client->canWrite = false;
      if (io_mod(client->fd, IO_READ | IO_WRITE | IO_EDGE_TRIGGERED, NULL,
                 server_onClientEvent)) {
        log_erro("server", "io_mod(): %d - %s.\n", errno, strerror(errno));
        server_close(client->fd);
        return;
      }
      break;
    }

    if (errno == EINTR) {
      continue;
    }

    log_erro("server", "client %d <<< error: %d - %s.\n", client->fd, errno,
             strerror(errno));

    server_close(client->fd);

    return;
  }

  if (client->readClosed) {
    log_dbug("server", "client %d <<< no more reading or writing, closing.\n",
             client->fd);
    server_close(client->fd);
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void server_read(Client *client) {
  log_dbug("server", "client %d >>> read\n", client->fd);

  if (!client->canRead) {
    log_dbug("server", "client %d >>> no pending data.\n", client->fd);
    return;
  }

  BuffWriter *writer = buff_writer(&client->inbox);

  while (!buff_writer_isfull(writer)) {
    ssize_t r =
        read(client->fd, buff_writer_data(writer), buff_writer_size(writer));

    if (r < 0) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        log_dbug("server", "client %d >>> can't read, try again.\n",
                 client->fd);
        client->canRead = false;
        break;
      } else {
        log_erro("server", "client %d >>> error: %d - %s\n", client->fd, errno,
                 strerror(errno));
        server_close(client->fd);
        return;
      }
    }

    if (r == 0) {
      log_dbug("server", "client %d >>> read closed\n", client->fd);
      client->readClosed = true;
      if (!client->busy && buff_isempty(&client->outbox) &&
          buff_isempty(&client->inbox)) {
        log_dbug("server", "client %d >>> no tasks, closing...\n", client->fd);
        server_close(client->fd);
        return;
      }
      break;
    }

    log_dbugbin("server", buff_writer_data(writer), r,
                "client %d >>> (%d bytes) ", client->fd, r);

    buff_writer_commit(writer, r);
  }

  server_processInbox(client);
}

////////////////////////////////////////////////////////////////////////////////

static void server_processInbox(Client *client) {
  // Ignora novas requisições enquanto houver uma requisição em andamento.
  if (client->busy) {
    log_dbug("server",
             "client %d >>> waiting for completion of the current request.\n",
             client->fd);
    return;
  }

  if (buff_isempty(&client->inbox)) {
    log_dbug("server", "client %d >>> inbox empty.\n", client->fd);
    return;
  }

  BuffReader *reader = buff_reader(&client->inbox);

  switch (server.params.onFormat(client->fd, reader)) {
    case FORMAT_OK:
      client->busy = true;
      server.params.onMessage(client->fd);
      break;
    case FORMAT_PART:
      break;
    case FORMAT_ERROR:
      log_erro("server", "client %d >>> onReceive() fail.\n", client->fd);
      server_close(client->fd);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void server_close(int clientFd) {
  server.numClients--;

  if (close(clientFd)) {
    log_erro("server", "close(): %d - %s.\n", errno, strerror(errno));
  }

  log_dbug("server", "Connection closed [%d/%d]: %d.\n", server.numClients,
           server.params.maxClients, clientFd);

  server.params.onDisconnected(clientFd);

  server_acceptClients();
}

////////////////////////////////////////////////////////////////////////////////

static Client *server_newClient() {
  Client *client = malloc(sizeof(Client));

  if (client == NULL) {
    log_erro("server", "malloc(): %d - %s\n", errno, strerror(errno));
    return NULL;
  }

  if (buff_init(&client->inbox, server.params.inboxMaxSize)) {
    log_erro("server", "buff_init()\n");
    free(client);
    return NULL;
  }

  if (buff_init(&client->outbox, server.params.inboxMaxSize)) {
    log_erro("server", "buff_init()\n");
    buff_free(&client->inbox);
    free(client);
    return NULL;
  }

  return client;
}

////////////////////////////////////////////////////////////////////////////////

// static void server_freeClient(Client *client) {
//   buff_free(&client->inbox);
//   buff_free(&client->outbox);
//   free(client);
// }

////////////////////////////////////////////////////////////////////////////////

void server_stop(int result) {
  log_info("server", "Stoping...\n");
  server.close = true;
  io_close(result);
}