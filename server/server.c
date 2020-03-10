#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
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
#include "str/str.h"
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
  IO *listen;
  IO *workers[8];
  thrd_t thWorkers[8];
} Server;

////////////////////////////////////////////////////////////////////////////////

typedef struct Client {
  int fd;
  str_t *outbox;
  bool canWrite;
  Buff inbox;
  bool busy;
  bool canRead;
  bool readClosed;
} Client;

////////////////////////////////////////////////////////////////////////////////

static Server server;

////////////////////////////////////////////////////////////////////////////////

static Client *server_newClient(int fd);
static Client *server_client(int fd);
static void server_read(Client *client);
static void server_write(Client *client);
static void server_onClientEvent(void *arg, int fd, IOEvent events);
static void server_acceptClients();
static void server_onListenEvent(void *arg, int fd, IOEvent events);
static void server_processInbox(Client *client);
static int server_setupSigTermHandler();
static void server_sigTermHandler(int signum, siginfo_t *info, void *ptr);
static int server_init(ServerParams params);
static void server_free();
static void server_freeClient(Client *client);
static int server_worker(void *arg);

////////////////////////////////////////////////////////////////////////////////

int server_start(ServerParams params) {
  if (server_init(params)) return -1;

  server.workers[0] = io_new();
  server.workers[1] = io_new();
  server.workers[2] = io_new();
  server.workers[3] = io_new();
  server.workers[4] = io_new();
  server.workers[5] = io_new();
  server.workers[6] = io_new();
  server.workers[7] = io_new();

  if (thrd_create(&server.thWorkers[0], server_worker, server.workers[0]))
    return -1;

  if (thrd_create(&server.thWorkers[1], server_worker, server.workers[1]))
    return -1;

  if (thrd_create(&server.thWorkers[2], server_worker, server.workers[2]))
    return -1;

  if (thrd_create(&server.thWorkers[3], server_worker, server.workers[3]))
    return -1;

  if (thrd_create(&server.thWorkers[4], server_worker, server.workers[4]))
    return -1;

  if (thrd_create(&server.thWorkers[5], server_worker, server.workers[5]))
    return -1;

  if (thrd_create(&server.thWorkers[6], server_worker, server.workers[6]))
    return -1;

  if (thrd_create(&server.thWorkers[7], server_worker, server.workers[7]))
    return -1;

  int r = io_run(server.listen, 100);

  server_free();

  log_info("server", "Bye!\n");

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static int server_worker(void *arg) {
  IO *io = arg;
  int maxClients = server.params.maxClients / 8;

  log_info("server-worker", "Started: %ld, maxClients: %d\n", thrd_current(),
           maxClients);

  int r = io_run(io, maxClients);

  log_info("server-worker", "Closed: %ld\n", thrd_current());

  return r;
}

////////////////////////////////////////////////////////////////////////////////

static int server_init(ServerParams params) {
  log_info("server", "Initializing...\n");

  server.params = params;
  server.fd = -1;
  server.canAccept = false;
  server.numClients = 0;
  server.clients = vetor_criar(params.maxClients);

  if (server.clients == NULL) goto error;

  struct sockaddr_in address = {0};
  address.sin_addr.s_addr = htonl(INADDR_ANY);
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

  log_info("server", "Concurrency max: %d\n", params.maxClients);
  log_info("server", "Waiting for connections on port: %d.\n", params.port);

  server.listen = io_new();

  if (io_add(server.listen, server.fd, IO_READ | IO_EDGE_TRIGGERED, NULL,
             server_onListenEvent)) {
    goto error;
  }

  if (server_setupSigTermHandler()) goto error;

  return 0;

error:
  if (server.fd != -1) close(server.fd);
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

static void server_free() {
  for (int i = 0; i < vetor_qtd(server.clients); i++) {
    Client *client = vetor_item(server.clients, i);
    server_freeClient(client);
    vetor_inserir(server.clients, i, NULL);
  }

  vetor_destruir(server.clients);
  server.clients = NULL;

  io_close(server.workers[0], 0);
  io_close(server.workers[1], 0);
  io_close(server.workers[2], 0);
  io_close(server.workers[3], 0);
  io_close(server.workers[4], 0);
  io_close(server.workers[5], 0);
  io_close(server.workers[6], 0);
  io_close(server.workers[7], 0);
}

////////////////////////////////////////////////////////////////////////////////

static int server_setupSigTermHandler() {
  static struct sigaction _sigact;

  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = server_sigTermHandler;
  _sigact.sa_flags = SA_SIGINFO;

  if (sigaction(SIGTERM, &_sigact, NULL)) {
    log_erro("server", "sigaction(): %d - %s\n", errno, strerror(errno));
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void server_sigTermHandler(int signum, siginfo_t *info, void *ptr) {
  server_stop(0);
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

    Client *client = server_newClient(clientFd);

    if (client == NULL) {
      log_erro("server", "client %d >>> server_newClient()\n", clientFd);
      server_close(clientFd);
      break;
    }

    server.params.onConnected(client->fd);

    server.params.onClean(client->fd);

    IO *worker = server.workers[server.numClients % 8];

    if (io_add(worker, client->fd, IO_READ | IO_EDGE_TRIGGERED, NULL,
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

static Client *server_newClient(int fd) {
  Client *client = vetor_item(server.clients, fd);

  if (client == NULL) {
    client = malloc(sizeof(Client));

    if (client == NULL) {
      log_erro("server", "malloc(): %d - %s\n", errno, strerror(errno));
      return NULL;
    }

    if (buff_init(&client->inbox, server.params.inboxMaxSize)) {
      log_erro("server", "buff_init()\n");
      free(client);
      return NULL;
    }

    if ((client->outbox = str_new(server.params.outboxInitSize)) == NULL) {
      log_erro("server", "str_new()\n");
      buff_free(&client->inbox);
      free(client);
      return NULL;
    }

    vetor_inserir(server.clients, fd, client);
  }

  client->fd = fd;
  client->canRead = false;
  client->readClosed = false;
  client->canWrite = true;
  client->busy = false;
  buff_clear(&client->inbox);
  str_clear(client->outbox);

  return client;
}

////////////////////////////////////////////////////////////////////////////////

static void server_freeClient(Client *client) {
  if (client == NULL) return;
  buff_free(&client->inbox);
  str_free(&client->outbox);
  free(client);
}

////////////////////////////////////////////////////////////////////////////////

static Client *server_client(int fd) { return vetor_item(server.clients, fd); }

////////////////////////////////////////////////////////////////////////////////

void server_send(int clientFd, const void *buff, size_t size) {
  Client *client = server_client(clientFd);

  if (str_addcstrlen(&client->outbox, buff, size)) {
    log_erro("server", "str_addcstrlen()\n");
    server_close(clientFd);
    return;
  }

  server_write(client);
}

////////////////////////////////////////////////////////////////////////////////

static void server_onFlush(Client *client) {
  server.params.onClean(client->fd);
  server_processInbox(client);
}

////////////////////////////////////////////////////////////////////////////////

static void server_onClientEvent(void *arg, int fd, IOEvent events) {
  Client *client = server_client(fd);

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
  if (!client->canWrite) {
    log_dbug("server",
             "client %d >>> outbox is busy, waiting for the write signal.\n",
             client->fd);
    return;
  }

  while (true) {
    ssize_t nwritten =
        write(client->fd, str_cstr(client->outbox), str_len(client->outbox));

    if (nwritten >= 0) {
      str_rm(client->outbox, 0, nwritten);
      log_dbug("server", "client %d <<< (%d bytes)\n", client->fd, nwritten);
      if (str_len(client->outbox) == 0) {
        client->busy = false;
        server_onFlush(client);
        break;
      } else {
        continue;
      }
    }

    if (errno == EAGAIN) {
      log_dbug("server", "client %d <<< can't write, try again.\n", client->fd);
      client->canWrite = false;
      if (io_mod(io_current(), client->fd,
                 IO_READ | IO_WRITE | IO_EDGE_TRIGGERED, NULL,
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
      if (!client->busy && str_len(client->outbox) == 0 &&
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

void server_stop(int result) {
  log_info("server", "Stoping...\n");
  server.close = true;
  io_close(server.listen, result);
}
