#include "server.h"
#include "connection_handler.h"
#include "thread_pool.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define POOL_SIZE 2
#define MAX_EVENTS 10

extern int client_counter;

void handle_task(void *arg) {
  int conn = *(int *)arg;
  free(arg); // Free the allocated memory for the connection descriptor
  handle_connection(conn);
}

void *handle_connection_wrapper(void *arg) {
  int conn = *(int *)arg;
  free(arg); // Free the allocated memory for the connection descriptor
  handle_connection(conn);
  return NULL;
}

void use_thread_pool(int server_fd) {
  thread_pool_t pool;
  thread_pool_init(&pool, POOL_SIZE);

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

    // Allocate memory for the connection descriptor and create a task
    int *conn_ptr = malloc(sizeof(int));
    *conn_ptr = conn;

    // Add the task to the task queue
    task_queue_push(&pool.task_queue, handle_task, conn_ptr);

    // printf("Client %d connected\n", current_client);
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

    // printf("Client %d connected\n", current_client);
  }
}

int set_non_blocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;

  fcntl(fd, F_SETFL, new_option);
  return 0;
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
  socklen_t client_addr_len = sizeof(client_addr);
  ;

  while (1) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n == -1) {
      perror("epoll_wait");
      break;
    }
    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == server_fd) {
        int connfd = accept(server_fd, (struct sockaddr *)&client_addr,
                            &client_addr_len);

        if (connfd == -1) {
          if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            break;
          } else {
            perror("accept");
            break;
          }
        }
        set_non_blocking(connfd);
        event.events = EPOLLIN;
        event.data.fd = connfd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &event) == -1) {
          perror("epoll_ctl");
          close(connfd);
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