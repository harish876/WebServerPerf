#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <sys/socket.h>

typedef enum { COMPLETE, WOULD_BLOCK, ERROR } CONNECTION_STATUS;

typedef struct {
  char method[16];
  char path[1024];
  char body[1024];
  size_t body_length;
} request_info;

CONNECTION_STATUS handle_connection(int conn);

#endif // CONNECTION_HANDLER_H