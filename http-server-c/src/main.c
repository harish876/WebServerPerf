#include "http_parser.h"
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Global client counter
int client_counter = 0;

typedef struct {
  char method[16];
  char path[1024];
  char body[1024];
  size_t body_length;
} request_info;

int on_url(http_parser *parser, const char *at, size_t length) {
  request_info *req_info = (request_info *)parser->data;
  strncpy(req_info->path, at, length);
  req_info->path[length] = '\0';
  return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
  request_info *req_info = (request_info *)parser->data;
  strncpy(req_info->body, at, length);
  req_info->body[length] = '\0';
  req_info->body_length = length;
  return 0;
}

int on_message_begin(http_parser *parser) {
  request_info *req_info = (request_info *)parser->data;
  memset(req_info, 0, sizeof(request_info)); // Clear previous request info
  return 0;
}

void handle_connection(int conn) {
  uint8_t buff[1024];
  ssize_t bytes_read = read(conn, buff, sizeof(buff));
  if (bytes_read == -1) {
    perror("read");
    close(conn);
    return; // Exit the function
  }
  buff[bytes_read] = '\0';

  http_parser parser;
  http_parser_init(&parser, HTTP_REQUEST);

  http_parser_settings settings;
  memset(&settings, 0, sizeof(settings));
  settings.on_url = on_url;
  settings.on_body = on_body;
  settings.on_message_begin = on_message_begin;

  request_info req_info;
  parser.data = &req_info;

  size_t parsed =
      http_parser_execute(&parser, &settings, (const char *)buff, bytes_read);
  if (parsed != bytes_read) {
    printf("Error parsing request\n");
    close(conn);
    return;
  }

  // Determine the request method
  const char *method = http_method_str((enum http_method)parser.method);
  strncpy(req_info.method, method, sizeof(req_info.method) - 1);

  if (strncmp(req_info.path, "/echo/", 6) == 0 &&
      strcmp(req_info.method, "GET") == 0) {
    char *content =
        req_info.path + 6; // 6 because thats the size of "/echo/" string
    size_t contentLength = strlen(content);
    char response[1024];
    int response_length =
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\nContent-Type: "
                 "text/plain\r\nContent-Length: %zu\r\n\r\n%s",
                 contentLength, content);
    send(conn, response, response_length, 0);
  } else if (strcmp(req_info.path, "/") == 0) {
    char response[] = "HTTP/1.1 200 OK\r\n\r\n";
    send(conn, response, sizeof(response) - 1, 0);
  } else {
    char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    send(conn, response, sizeof(response) - 1, 0);
  }

  close(conn);
}

void *handle_connection_wrapper(void *arg) {
  int conn = *(int *)arg;
  free(arg); // Free the allocated memory for the connection descriptor
  handle_connection(conn);
  return NULL;
}

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");

  int server_fd;
  socklen_t client_addr_len;
  struct sockaddr_in client_addr;
  pthread_t tid;

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

  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  // intresting feature look into this
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
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

  close(server_fd);

  return 0;
}
