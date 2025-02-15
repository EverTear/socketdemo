#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct cthread_pool_st cthread_pool_t;

extern cthread_pool_t* default_create_threadpool(void);

extern cthread_pool_t* create_threadpool(size_t min_thread_num, size_t max_thread_num);

extern int destroy_threadpool(cthread_pool_t* pool);

extern int add_job(cthread_pool_t* pool, void* (*process)(void* arg), void* arg);

#endif