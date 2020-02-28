#include <stdio.h>

#include "http.h"
#include "log/log.h"

static void handleTest(int clientFd) {
  str_t *body = str_clonecstr("<b>Olá</b>, Brasil!\n");
  
  http_sendStatus(clientFd, HTTP_STATUS_OK);
  http_sendType(clientFd, HTTP_TYPE_HTML);
  http_send(clientFd, body);

  str_free(&body);
}

int main() {

  log_ignore("io", LOG_TRAC);
  log_ignore("server", LOG_TRAC);
  log_ignore("http", LOG_INFO);

  http_handler("GET", "/test$", handleTest);
  

  return http_start(8080, 100);
}
