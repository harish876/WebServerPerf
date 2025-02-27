#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void task_queue_init(task_queue_t *queue, int size) {
  queue->tasks = malloc(size * sizeof(task_t));
  queue->front = 0;
  queue->rear = 0;
  queue->count = 0;
  queue->size = size;
  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->cond, NULL);
}

void task_queue_destroy(task_queue_t *queue) {
  free(queue->tasks);
  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->cond);
}

void task_queue_push(task_queue_t *queue, task_t task) {
  pthread_mutex_lock(&queue->mutex);
  while (queue->count == queue->size) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
  }
  queue->tasks[queue->rear] = task;
  queue->rear = (queue->rear + 1) % queue->size;
  queue->count++;
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
}

task_t task_queue_pop(task_queue_t *queue) {
  pthread_mutex_lock(&queue->mutex);
  while (queue->count == 0) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
  }
  task_t task = queue->tasks[queue->front];
  queue->front = (queue->front + 1) % queue->size;
  queue->count--;
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->mutex);
  return task;
}

void *worker_thread(void *arg) {
  thread_pool_t *pool = (thread_pool_t *)arg;
  while (1) {
    task_t task = task_queue_pop(&pool->task_queue);
    pool->handler(task.conn); // Call the registered handler function
  }
  return NULL;
}

void thread_pool_init(thread_pool_t *pool, int pool_size, int queue_size,
                      void (*handler)(int)) {
  pool->pool_size = pool_size;
  pool->threads = malloc(pool_size * sizeof(pthread_t));
  pool->handler = handler; // Register the handler function
  task_queue_init(&pool->task_queue, queue_size);
  for (int i = 0; i < pool_size; i++) {
    pthread_create(&pool->threads[i], NULL, worker_thread, pool);
  }
}

void thread_pool_destroy(thread_pool_t *pool) {
  for (int i = 0; i < pool->pool_size; i++) {
    pthread_cancel(pool->threads[i]);
    pthread_join(pool->threads[i], NULL);
  }
  free(pool->threads);
  task_queue_destroy(&pool->task_queue);
}