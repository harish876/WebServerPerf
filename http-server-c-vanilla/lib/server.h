#ifndef SERVER_MODES_H
#define SERVER_MODES_H

void use_thread_pool(int server_fd);
void use_threads(int server_fd);
void use_epoll(int server_fd);
int set_non_blocking(int sockfd);

#endif // SERVER_MODES_H