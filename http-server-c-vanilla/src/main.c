#include "connection_handler.h"
#include "thread_pool.h"
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define POOL_SIZE 32
#define QUEUE_SIZE 128
#define MAX_EVENTS 10

// Global client counter
int client_counter = 0;

void *handle_connection_wrapper(void *arg) {
  int conn = *(int *)arg;
  free(arg); // Free the allocated memory for the connection descriptor
  handle_connection(conn);
  return NULL;
}

void use_thread_pool(int server_fd) {
  thread_pool_t pool;
  thread_pool_init(&pool, POOL_SIZE, QUEUE_SIZE, handle_connection);

  struct sockaddr_in client_addr;
  socklen_t client_addr_len;

  while (1) {
    client_addr_len = sizeof(client_addr);
    int conn =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (conn < 0) {
      break;
    }

    // Increment the client counter
    client_counter++;
    int current_client = client_counter;

    // Create a task and add it to the task queue
    task_t task = {.conn = conn};
    task_queue_push(&pool.task_queue, task);

    printf("Client %d connected\n", current_client);
  }

  thread_pool_destroy(&pool);
}

void use_threads(int server_fd) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len;
  pthread_t tid;

  while (1) {
    client_addr_len = sizeof(client_addr);
    int conn =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (conn < 0) {
      break;
    }

    // Increment the client counter
    client_counter++;
    int current_client = client_counter;

    // Allocate memory for the connection descriptor and pass it to the thread
    int *conn_ptr = malloc(sizeof(int));
    *conn_ptr = conn;

    pthread_create(&tid, NULL, handle_connection_wrapper, conn_ptr);
    pthread_detach(tid); // Detach the thread to avoid memory leaks

    printf("Client %d connected\n", current_client);
  }
}

void use_epoll(int server_fd) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server_fd;

  // Add the server_fd to the epoll instance's interest list
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
    perror("epoll_ctl");
    exit(EXIT_FAILURE);
  }

  struct epoll_event events[MAX_EVENTS];
  struct sockaddr_in client_addr;
  socklen_t client_addr_len;

  while (1) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == server_fd) {
        client_addr_len = sizeof(client_addr);
        int conn = accept(server_fd, (struct sockaddr *)&client_addr,
                          &client_addr_len);
        if (conn < 0) {
          perror("accept");
          continue;
        }

        event.events = EPOLLIN | EPOLLET;
        event.data.fd = conn;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn, &event) == -1) {
          perror("epoll_ctl");
          close(conn);
          continue;
        }
      } else {
        handle_connection(events[i].data.fd);
        close(events[i].data.fd);
      }
    }
  }
  close(epoll_fd);
}

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

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");

  if (strcmp(argv[1], "thread_pool") == 0) {
    use_thread_pool(server_fd);
  } else if (strcmp(argv[1], "threads") == 0) {
    use_threads(server_fd);
  } else if (strcmp(argv[1], "epoll") == 0) {
    use_epoll(server_fd);
  } else {
    fprintf(stderr, "Invalid mode: %s\n", argv[1]);
    fprintf(stderr, "Modes: thread_pool, threads, epoll\n");
    return 1;
  }

  close(server_fd);

  return 0;
}
