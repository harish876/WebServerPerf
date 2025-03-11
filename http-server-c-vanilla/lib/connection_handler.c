#include "connection_handler.h"
#include "yyjson.h"
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

void handle_echo_endpoint(int conn, const char *content) {
  size_t contentLength = strlen(content);
  char response[1024];
  int response_length = snprintf(response, sizeof(response),
                                 "HTTP/1.1 200 OK\r\nContent-Type: "
                                 "text/plain\r\nContent-Length: %zu\r\n\r\n%s",
                                 contentLength, content);
  send(conn, response, response_length, 0);
}

void handle_json_endpoint(int conn, const char *body) {
  yyjson_doc *doc = yyjson_read(body, strlen(body), 0);
  if (!doc) {
    const char *response = "HTTP/1.1 400 Bad Request\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: 15\r\n"
                           "\r\n"
                           "{\"error\":\"Invalid JSON\"}";
    send(conn, response, strlen(response), 0);
    close(conn);
    return;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  yyjson_val *id_val = yyjson_obj_get(root, "id");

  if (id_val && yyjson_is_uint(id_val)) {
    uint64_t id = yyjson_get_uint(id_val);
    id++;
    yyjson_mut_doc *mut_doc = yyjson_doc_mut_copy(doc, NULL);
    yyjson_mut_val *mut_root = yyjson_mut_doc_get_root(mut_doc);

    // Update the existing id field
    yyjson_mut_val *mut_id_val = yyjson_mut_obj_get(mut_root, "id");
    yyjson_mut_set_uint(mut_id_val, id);

    char *json_response = yyjson_mut_write(mut_doc, 0, NULL);
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    char response[1024];

    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(json_response), json_response);
    send(conn, response, strlen(response), 0);
    free(json_response);
  } else {
    yyjson_doc_free(doc);
    char response[1024];
    char *json_response = "{\"error\":\"Invalid Object. Missing 'id' field or "
                          "'id' field should be int\"}";
    snprintf(response, sizeof(response),
             "HTTP/1.1 400 Bad Request\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(json_response), json_response);
    send(conn, response, strlen(response), 0);
  }
}

void handle_root_endpoint(int conn) {
  char response[] = "HTTP/1.1 200 OK\r\n\r\n";
  send(conn, response, sizeof(response) - 1, 0);
}

void handle_not_found_endpoint(int conn) {
  char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
  send(conn, response, sizeof(response) - 1, 0);
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
  printf("Request: %s", buff);

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
    handle_echo_endpoint(conn, req_info.path + 6);
  } else if (strcmp(req_info.path, "/json") == 0 &&
             strcmp(req_info.method, "POST") == 0) {
    handle_json_endpoint(conn, req_info.body);
  } else if (strcmp(req_info.path, "/") == 0) {
    handle_root_endpoint(conn);
  } else {
    handle_not_found_endpoint(conn);
  }
#else
  char method[16], path[1024];
  sscanf((char *)buff, "%s %s", method, path);

  if (strncmp(path, "/echo/", 6) == 0 && strcmp(method, "GET") == 0) {
    handle_echo_endpoint(conn, path + 6);
  } else if (strcmp(path, "/json") == 0 && strcmp(method, "POST") == 0) {
    char *body_start = strstr((char *)buff, "\r\n\r\n");
    if (body_start) {
      body_start += 4; // Skip the "\r\n\r\n" sequence
      handle_json_endpoint(conn, body_start);
    } else {
      handle_not_found_endpoint(conn);
    }
  } else if (strcmp(path, "/") == 0) {
    handle_root_endpoint(conn);
  } else {
    handle_not_found_endpoint(conn);
  }
#endif

  close(conn);
}