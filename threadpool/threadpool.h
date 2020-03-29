#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct ThreadPool ThreadPool;

ThreadPool *threadpool_create(int size);

void threadpool_execute(ThreadPool *pool, void(*handler)(void *arg), void *arg);

void threadpool_destroy(ThreadPool *pool);

#endif