#ifndef SERVER_H
#define SERVER_H

#include "buff/buff.h"

typedef enum FormatStatus {
  FORMAT_OK,
  FORMAT_PART,
  FORMAT_ERROR,
} FormatStatus;

typedef void (*ServerOnConnected)(int client);

typedef void (*ServerOnDisconnected)(int client);

typedef void (*ServerOnMessage)(int client);

typedef void (*ServerOnClean)(int client);

typedef FormatStatus (*ServerOnFormat)(int client, BuffReader *reader);

typedef struct ServerParams {
  int port;
  char *host;
  int maxClients;
  int inboxMaxSize;
  int outboxInitSize;
  ServerOnFormat onFormat;
  ServerOnMessage onMessage;
  ServerOnConnected onConnected;
  ServerOnDisconnected onDisconnected;
  ServerOnClean onClean;
} ServerParams;

int server_start(ServerParams params);

void server_send(int clientFd, const void *buff, size_t size);

void server_close(int clientFd);

void server_stop(int result);

#endif