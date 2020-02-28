#include <stdio.h>

#include "http.h"

static void handleTest(int clientFd) {
  str_t *body = str_clonecstr("<b>Olá</b>, Brasil!");
  
  http_sendStatus(clientFd, HTTP_STATUS_OK);
  http_sendType(clientFd, HTTP_TYPE_HTML);
  http_send(clientFd, body);

  str_free(&body);
}

int main() {


  http_handler("GET", "/test$", handleTest);
  

  return http_start(8080, 100);
}