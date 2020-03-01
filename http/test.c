#include <stdio.h>
#include <string.h>

#include "http.h"
#include "log/log.h"

static void handleTest(int clientFd) {
  const char *body = "<b>Olá</b>, Brasil!\n";

  http_sendStatus(clientFd, HTTP_STATUS_OK);
  http_sendType(clientFd, HTTP_TYPE_HTML);
  http_send(clientFd, body, strlen(body));
}

int main() {
  log_ignore("io", LOG_TRAC);
  log_ignore("server", LOG_TRAC);
  log_ignore("http", LOG_INFO);

  http_handler("GET", "/test$", handleTest);

  return http_start(8282, 1000);
}
