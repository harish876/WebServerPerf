#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "http_parser.h"
#include <sys/socket.h>

typedef struct {
  char method[16];
  char path[1024];
  char body[1024];
  size_t body_length;
} request_info;

int on_url(http_parser *parser, const char *at, size_t length);
int on_body(http_parser *parser, const char *at, size_t length);
int on_message_begin(http_parser *parser);
void handle_connection(int conn);

#endif // CONNECTION_HANDLER_H