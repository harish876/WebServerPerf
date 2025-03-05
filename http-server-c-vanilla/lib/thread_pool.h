#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct task {
  void (*function)(void *);
  void *arg;
  struct task *next;
} task_t;

typedef struct task_queue {
  task_t *front;
  task_t *rear;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} task_queue_t;

typedef struct thread_pool {
  pthread_t *threads;
  int pool_size;
  task_queue_t task_queue;
  int shutdown;
} thread_pool_t;

void task_queue_init(task_queue_t *queue);
void task_queue_destroy(task_queue_t *queue);
void task_queue_push(task_queue_t *queue, void (*function)(void *), void *arg);
task_t *task_queue_pop(task_queue_t *queue);

void thread_pool_init(thread_pool_t *pool, int pool_size);
void thread_pool_destroy(thread_pool_t *pool);
void thread_pool_execute(thread_pool_t *pool, void (*function)(void *),
                         void *arg);

#endif // THREAD_POOL_H