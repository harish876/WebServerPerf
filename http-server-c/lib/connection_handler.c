#include "connection_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if USE_HTTP_PARSER
#include "http_parser.h"

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
#endif

void handle_connection(int conn) {
  uint8_t buff[1024];
  ssize_t bytes_read = read(conn, buff, sizeof(buff));
  if (bytes_read == -1) {
    perror("read");
    close(conn);
    return; // Exit the function
  }
  buff[bytes_read] = '\0';

#if USE_HTTP_PARSER
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
        req_info.path + 6; // 6 because that's the size of "/echo/" string
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
#else
  char method[16], path[1024];
  sscanf((char *)buff, "%s %s", method, path);

  // printf("Methods: %s Path: %s\n", method, path);
  if (strncmp(path, "/echo/", 6) == 0 && strcmp(method, "GET") == 0) {
    char *content = path + 6; // 6 because that's the size of "/echo/" string
    size_t contentLength = strlen(content);
    char response[1024];
    int response_length =
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\nContent-Type: "
                 "text/plain\r\nContent-Length: %zu\r\n\r\n%s",
                 contentLength, content);
    send(conn, response, response_length, 0);
  } else if (strcmp(path, "/") == 0) {
    char response[] = "HTTP/1.1 200 OK\r\n\r\n";
    send(conn, response, sizeof(response) - 1, 0);
  } else {
    char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    send(conn, response, sizeof(response) - 1, 0);
  }
#endif

  close(conn);
}