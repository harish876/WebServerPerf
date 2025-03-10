#include "server.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 128 // how many pending connections queue will hold

// Global client counter
int client_counter = 0;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <mode>\n", argv[0]);
    fprintf(stderr, "Modes: thread_pool, threads, epoll\n");
    return 1;
  }

  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");

  int server_fd;
  struct sockaddr_in serv_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(4221);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  if (listen(server_fd, BACKLOG) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");

  if (strcmp(argv[1], "thread_pool") == 0) {
    use_thread_pool(server_fd);
  } else if (strcmp(argv[1], "threads") == 0) {
    use_threads(server_fd);
  } else if (strcmp(argv[1], "epoll") == 0) {
    set_non_blocking(server_fd);
    use_epoll(server_fd);
  } else {
    fprintf(stderr, "Invalid mode: %s\n", argv[1]);
    fprintf(stderr, "Modes: thread_pool, threads, epoll\n");
    return 1;
  }

  close(server_fd);

  return 0;
}