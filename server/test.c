#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buff/buff.h"
#include "log/log.h"
#include "tcp/server.h"

#define COMANDO_MAX_SIZE 128
#define STATUS_MAX_SIZE 64

////////////////////////////////////////////////////////////////////////////////

typedef enum ClientState {
  CLIENT_STATE_INIT,
  CLIENT_STATE_CMD,
  CLIENT_STATE_END,
  CLIENT_STATE_ERROR,
} ClientState;

////////////////////////////////////////////////////////////////////////////////

typedef struct MsgReq {
  char comando[COMANDO_MAX_SIZE];
  int comandoLen;
} MsgReq;

////////////////////////////////////////////////////////////////////////////////

typedef struct MsgResp {
  char status[STATUS_MAX_SIZE];
} MsgResp;

////////////////////////////////////////////////////////////////////////////////

typedef struct Client {
  int id;
  ClientState state;
  MsgReq req;
  MsgResp resp;
} Client;

////////////////////////////////////////////////////////////////////////////////

static Client *arrClients = NULL;
static int arrClientsSize = 0;

////////////////////////////////////////////////////////////////////////////////

static bool isCommandChar(char c);
static bool isSpaceChar(char c);
static void onConnected(int clientId);
static void onDisconnected(int clientId);
static FormatStatus onFormat(int clientId, BuffReader *reader);
static void onMessage(int clientId);

////////////////////////////////////////////////////////////////////////////////

int main() {
  ServerParams params;
  params.port = 2000;
  params.host = "127.0.0.1";
  params.maxClients = 10;
  params.inboxMaxSize = 4096;
  params.onConnected = onConnected;
  params.onFormat = onFormat;
  params.onMessage = onMessage;
  params.onDisconnected = onDisconnected;

  return server_start(params);
}

////////////////////////////////////////////////////////////////////////////////

static void onConnected(int clientId) {
  if (clientId >= arrClientsSize) {
    arrClientsSize = clientId + 1;
    arrClients = realloc(arrClients, sizeof(Client) * arrClientsSize);
  }
  arrClients[clientId].id = clientId;
  arrClients[clientId].state = CLIENT_STATE_INIT;
}

////////////////////////////////////////////////////////////////////////////////

static void onDisconnected(int clientId) {}

////////////////////////////////////////////////////////////////////////////////

static FormatStatus onFormat(int clientId, BuffReader *reader) {
  Client *client = &arrClients[clientId];
  const char *ptrC = NULL;
  char c;
  size_t r;

  while ((r = buff_reader_read(reader, &ptrC, 1)) > 0) {
    c = *ptrC;
    switch (client->state) {
      case CLIENT_STATE_INIT:
        client->req.comando[0] = '\0';
        client->req.comandoLen = 0;
        if (isSpaceChar(c)) {
          // Ignore
        } else if (isCommandChar(c)) {
          client->req.comando[client->req.comandoLen++] = c;
          client->req.comando[client->req.comandoLen] = '\0';
          client->state = CLIENT_STATE_CMD;
        } else {
          client->state = CLIENT_STATE_ERROR;
        }
        break;

      case CLIENT_STATE_CMD:
        if (isCommandChar(c)) {
          client->req.comando[client->req.comandoLen++] = c;
          client->req.comando[client->req.comandoLen] = '\0';
        } else if (isSpaceChar(c)) {
          client->state = CLIENT_STATE_END;
        } else {
          client->state = CLIENT_STATE_ERROR;
        }
        break;

      case CLIENT_STATE_END:
        client->state = CLIENT_STATE_INIT;
        return FORMAT_OK;

      case CLIENT_STATE_ERROR:
      default:
        client->state = CLIENT_STATE_INIT;
        return FORMAT_ERROR;
        break;
    }
  }

  return FORMAT_PART;
}

////////////////////////////////////////////////////////////////////////////////

static bool isCommandChar(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || (c == '-' || c == '.');
}

////////////////////////////////////////////////////////////////////////////////

static bool isSpaceChar(char c) { return c == ' ' || c == '\n' || c == '\t'; }

////////////////////////////////////////////////////////////////////////////////

static void send(Client *client) {
  char buff[128] = {0};

  int buffLen = snprintf(buff, sizeof(buff), "%s ", client->resp.status);

  server_send(client->id, buff, buffLen);
}

////////////////////////////////////////////////////////////////////////////////

static void onMessage(int clientId) {
  Client *client = &arrClients[clientId];

  log_info("test", "executing command: %s\n.", client->req.comando);

  strcpy(client->resp.status, "ok");

  send(client);
}
