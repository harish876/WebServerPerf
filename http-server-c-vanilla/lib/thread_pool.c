#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void task_queue_init(task_queue_t *queue) {
  queue->front = NULL;
  queue->rear = NULL;
  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->cond, NULL);
}

void task_queue_destroy(task_queue_t *queue) {
  while (queue->front != NULL) {
    task_t *temp = queue->front;
    queue->front = queue->front->next;
    free(temp);
  }
  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->cond);
}

void task_queue_push(task_queue_t *queue, void (*function)(void *), void *arg) {
  task_t *new_task = malloc(sizeof(task_t));
  if (new_task == NULL) {
    perror("Failed to allocate memory for new task");
    exit(EXIT_FAILURE);
  }
  new_task->function = function;
  new_task->arg = arg;
  new_task->next = NULL;

  pthread_mutex_lock(&queue->mutex);
  if (queue->rear == NULL) {
    queue->front = new_task;
    queue->rear = new_task;
  } else {
    queue->rear->next = new_task;
    queue->rear = new_task;
  }
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
}

task_t *task_queue_pop(task_queue_t *queue) {
  pthread_mutex_lock(&queue->mutex);
  while (queue->front == NULL) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
  }
  task_t *task = queue->front;
  queue->front = queue->front->next;
  if (queue->front == NULL) {
    queue->rear = NULL;
  }
  pthread_mutex_unlock(&queue->mutex);
  return task;
}

void *worker_thread(void *arg) {
  thread_pool_t *pool = (thread_pool_t *)arg;
  while (1) {
    task_t *task = task_queue_pop(&pool->task_queue);
    if (task == NULL) {
      continue;
    }
    task->function(task->arg);
    free(task);
  }
  return NULL;
}

void thread_pool_init(thread_pool_t *pool, int pool_size) {
  pool->pool_size = pool_size;
  pool->threads = malloc(pool_size * sizeof(pthread_t));
  pool->shutdown = 0;
  task_queue_init(&pool->task_queue);
  for (int i = 0; i < pool_size; i++) {
    pthread_create(&pool->threads[i], NULL, worker_thread, pool);
  }
}

void thread_pool_destroy(thread_pool_t *pool) {
  pool->shutdown = 1;
  for (int i = 0; i < pool->pool_size; i++) {
    pthread_cancel(pool->threads[i]);
    pthread_join(pool->threads[i], NULL);
  }
  free(pool->threads);
  task_queue_destroy(&pool->task_queue);
}

void thread_pool_execute(thread_pool_t *pool, void (*function)(void *),
                         void *arg) {
  task_queue_push(&pool->task_queue, function, arg);
}