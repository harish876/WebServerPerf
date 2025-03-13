#include <stddef.h>

#ifndef SERVER_MODES_H
#define SERVER_MODES_H

#define BUFFER_SIZE 1024

typedef struct {
  char method[16];
  char endpoint[256];
  size_t content_length;
  char accept_encoding[256];
  char content_type[256];
  char body[BUFFER_SIZE];
} RequestContext;

typedef enum { READ, PROCESS, WRITE, FLUSH } ConnectionState;

typedef struct {
  int fd;
  ConnectionState state;
  char buffer[BUFFER_SIZE];
  size_t read;
  RequestContext context;
  char *response;
  size_t response_len;
  size_t written;
} Connection;

void use_thread_pool(int server_fd);
void use_threads(int server_fd);
void use_epoll(int server_fd);
int set_non_blocking(int sockfd);

#endif // SERVER_MODES_H