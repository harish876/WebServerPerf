#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct {
  int conn;
} task_t;

typedef struct {
  task_t *tasks;
  int front;
  int rear;
  int count;
  int size;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} task_queue_t;

typedef struct {
  pthread_t *threads;
  int pool_size;
  task_queue_t task_queue;
  void (*handler)(int); // Function pointer for the handler function
} thread_pool_t;

void task_queue_init(task_queue_t *queue, int size);
void task_queue_destroy(task_queue_t *queue);
void task_queue_push(task_queue_t *queue, task_t task);
task_t task_queue_pop(task_queue_t *queue);

void thread_pool_init(thread_pool_t *pool, int pool_size, int queue_size,
                      void (*handler)(int));
void thread_pool_destroy(thread_pool_t *pool);

#endif // THREAD_POOL_H