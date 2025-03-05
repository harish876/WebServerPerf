/*
     This file is part of libmicrohttpd
     (C) 2007 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
   USA
*/
/**
 * @file minimal_example.c
 * @brief minimal example for how to use libmicrohttpd
 * @author Christian Grothoff
 */

#include "stdio.h"
#include "string.h"
#include "yyjson.h"
#include <microhttpd.h>
#include <netinet/in.h>
#include <stdlib.h>

#define PAGE                                                                   \
  "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd "   \
  "demo</body></html>"

#define PORT 4221

struct connection_info_struct {
  int connectiontype;
  struct MHD_PostProcessor *postprocessor;
  char *post_data;
  size_t post_data_size;
};

static int send_response(struct MHD_Connection *connection,
                         const char *response_text, int status_code,
                         const char *content_type) {
  int ret;
  struct MHD_Response *response;
  response = MHD_create_response_from_buffer(
      strlen(response_text), (void *)response_text, MHD_RESPMEM_MUST_FREE);
  if (!response) {
    return MHD_NO;
  }

  if (content_type) {
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE,
                            content_type);
  }

  ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);
  return ret;
}

static enum MHD_Result echo_endpoint(struct MHD_Connection *connection,
                                     const char *url, const char *method) {
  const char *content = url + 6; // 6 because that's the size of "/echo/" string
  return send_response(connection, strdup(content), MHD_HTTP_OK, "text/plain");
}

static enum MHD_Result json_endpoint(struct MHD_Connection *connection,
                                     const char *method,
                                     const char *upload_data,
                                     size_t *upload_data_size,
                                     struct connection_info_struct *con_info) {
  if (*upload_data_size != 0) {
    con_info->post_data = realloc(
        con_info->post_data, con_info->post_data_size + *upload_data_size + 1);
    memcpy(con_info->post_data + con_info->post_data_size, upload_data,
           *upload_data_size);
    con_info->post_data_size += *upload_data_size;
    con_info->post_data[con_info->post_data_size] = '\0';
    *upload_data_size = 0;
    return MHD_YES;
  } else {
    yyjson_doc *doc =
        yyjson_read(con_info->post_data, con_info->post_data_size, 0);
    if (!doc) {
      return send_response(connection, strdup("Invalid JSON"),
                           MHD_HTTP_BAD_REQUEST, "application/json");
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
      return send_response(connection, json_response, MHD_HTTP_OK,
                           "application/json");
    } else {
      yyjson_doc_free(doc);
      return send_response(
          connection, strdup("Invalid JSON: Missing or invalid 'id' field"),
          MHD_HTTP_BAD_REQUEST, "application/json");
    }
  }
}

static enum MHD_Result
handle_request(void *cls, struct MHD_Connection *connection, const char *url,
               const char *method, const char *version, const char *upload_data,
               size_t *upload_data_size, void **con_cls) {
  static int aptr;
  struct connection_info_struct *con_info;

  if (NULL == *con_cls) {
    con_info = malloc(sizeof(struct connection_info_struct));
    con_info->post_data = NULL;
    con_info->post_data_size = 0;
    *con_cls = con_info;
    return MHD_YES;
  }

  con_info = *con_cls;

  if (0 == strcmp(method, "GET")) {
    if (strncmp(url, "/echo/", 6) == 0) {
      return echo_endpoint(connection, url, method);
    } else if (strcmp(url, "/") == 0) {
      return send_response(connection, strdup(PAGE), MHD_HTTP_OK, "text/html");
    } else {
      return send_response(connection, strdup("Not Found"), MHD_HTTP_NOT_FOUND,
                           "text/plain");
    }
  } else if (0 == strcmp(method, "POST")) {
    if (strcmp(url, "/json") == 0) {
      return json_endpoint(connection, method, upload_data, upload_data_size,
                           con_info);
    } else {
      return send_response(connection, strdup("Not Found"), MHD_HTTP_NOT_FOUND,
                           "text/plain");
    }
  } else {
    return send_response(connection, strdup("Unexpected method"),
                         MHD_HTTP_METHOD_NOT_ALLOWED, "text/plain");
  }
}

void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe) {
  struct connection_info_struct *con_info = *con_cls;
  if (con_info) {
    if (con_info->post_data) {
      free(con_info->post_data);
    }
    free(con_info);
  }
  *con_cls = NULL;
}

void handler(int sig) { printf("Received signal: %d\n", sig); }

int main(int argc, char *const *argv) {
  struct MHD_Daemon *d;
  struct sockaddr_in serv_addr;

  // Set up the sockaddr_in structure to bind to 0.0.0.0
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
  serv_addr.sin_port = htons(PORT);

  // d = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY | MHD_USE_DEBUG,
  // atoi(argv[1]),
  //                      NULL, NULL, &handle_request, PAGE,
  //                      MHD_OPTION_SOCK_ADDR, &serv_addr, MHD_OPTION_END);
  d = MHD_start_daemon(MHD_USE_EPOLL_INTERNAL_THREAD | MHD_USE_DEBUG, PORT,
                       NULL, NULL, &handle_request, PAGE, MHD_OPTION_SOCK_ADDR,
                       &serv_addr, MHD_OPTION_NOTIFY_COMPLETED,
                       request_completed, NULL, MHD_OPTION_CONNECTION_TIMEOUT,
                       (unsigned int)10, MHD_OPTION_END);
  if (d == NULL) {
    fprintf(stdout, "Failed to start daemon\n");
    return 1;
  }

  printf("Server started on port %d...\n", PORT);
  (void)getc(stdin);
  MHD_stop_daemon(d);
  return 0;
}