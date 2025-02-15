#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define DEFAULT_MIN_THREAD_NUM          32
#define DEFAULT_MAX_THREAD_NUM          32

typedef struct job_st{
    void* (*process)(void* arg);    // callback function
    void* arg;  // callback function arguments
    struct job_st* next; // next worker
}job_t;

typedef struct cthread_pool_st{
    pthread_mutex_t queue_lock; // mutex
    pthread_cond_t queue_ready; // condition variable
    job_t* queue_head; // queue head node
    unsigned char shutdown; // thread pool destruction flag
    pthread_t* threadid; // thread id
    size_t max_thread_num;
    size_t max_free_thread_num;
    size_t min_thread_num;
    size_t current_thread_num;
    size_t working_thread_num;
    size_t waiting_job_num;
    pthread_t status_thread;
    int ms_interval;
}cthread_pool_t;

static void* _threadpool_routine(void* arg){
    cthread_pool_t* pool = (cthread_pool_t*)arg;
    job_t* worker = NULL;

    pthread_mutex_lock(&(pool->queue_lock));
    pool->current_thread_num += 1;
    pthread_mutex_unlock(&(pool->queue_lock));

    while(1){
        pthread_mutex_lock(&(pool->queue_lock));
        while((pool->waiting_job_num == 0) && (!pool->shutdown)){
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }

        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit(NULL);
        }

        worker = pool->queue_head;
        pool->queue_head = worker->next;
        pool->waiting_job_num -= 1;
        pool->working_thread_num += 1;
        pthread_mutex_unlock(&(pool->queue_lock));

        (*(worker->process))(worker->arg);

        pthread_mutex_lock(&(pool->queue_lock));
        pool->working_thread_num -= 1;
        free(worker);
        worker = NULL;

        if((pool->current_thread_num - pool->working_thread_num) > pool->max_free_thread_num && pool->current_thread_num > pool->min_thread_num){
            pthread_mutex_unlock(&(pool->queue_lock));
            break;
        }

        pthread_mutex_unlock(&(pool->queue_lock));
    }

    pthread_mutex_lock(&(pool->queue_lock));
    pool->current_thread_num -= 1;
    pthread_mutex_unlock(&(pool->queue_lock));

    pthread_exit(NULL);
}

static int _update_threadpool_max_free(cthread_pool_t* pool){
    size_t new_max_free_num = 0;
    pthread_mutex_lock(&(pool->queue_lock));
    new_max_free_num = pool->current_thread_num / 10;
    if((pool->max_thread_num > 0) && (pool->current_thread_num + new_max_free_num > pool->max_thread_num)){
        new_max_free_num = pool->max_thread_num - pool->current_thread_num;
    }
    pool->max_free_thread_num = new_max_free_num;
    pthread_mutex_unlock(&(pool->queue_lock));
    return 0;
}

cthread_pool_t* create_threadpool(size_t min_thread_num, size_t max_thread_num){
    size_t i = 0;
    cthread_pool_t* pool = (cthread_pool_t*)calloc(1, sizeof(cthread_pool_t));
    if(NULL == pool){
        perror("calloc error");
        goto fail;
    }

    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_ready), NULL);

    pthread_mutex_lock(&(pool->queue_lock));
    pool->max_thread_num = max_thread_num;
    pool->ms_interval = -1;
    pool->min_thread_num = min_thread_num;
    pthread_mutex_unlock(&(pool->queue_lock));

    pool->threadid = (pthread_t*)calloc(max_thread_num, sizeof(pthread_t));
    if(NULL == pool->threadid){
        perror("calloc error");
        goto fail;
    }
    for(i = 0; i < min_thread_num; ++i){
        pthread_create(&(pool->threadid[i]), NULL, _threadpool_routine, (void*)pool);
        // usleep(1000);
    }

    _update_threadpool_max_free(pool);

    return pool;

fail:
    if(pool != NULL){
        free(pool);
    }
    if(pool->threadid != NULL){
        free(pool->threadid);
    }

    return NULL;
}

cthread_pool_t* default_create_threadpool(void){
    return create_threadpool(DEFAULT_MIN_THREAD_NUM, DEFAULT_MAX_THREAD_NUM);
}

int destroy_threadpool(cthread_pool_t* pool){
    size_t i = 0;
    job_t* head = NULL;

    if(pool->shutdown){
        return -1;
    }

    pool->shutdown = 1;

    pthread_cond_broadcast(&(pool->queue_ready));
    for(i = 0; i < pool->current_thread_num; ++i){
        pthread_join(pool->threadid[i], NULL);
    }

    free(pool->threadid);

    while(NULL != pool->queue_head){
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(head);
    }

    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free(pool);
    pool = NULL;

    return 0;
}

int add_job(cthread_pool_t* pool, void* (*process)(void* arg), void* arg){
    job_t* new_job = NULL;
    job_t* member = NULL;
    size_t free_thread_num = 0;
    size_t i = 0;

    new_job = (job_t*)calloc(1, sizeof(job_t));
    if(NULL == new_job){
        perror("calloc error");
        return -1;
    }

    new_job->process = process;
    new_job->arg = arg;
    new_job->next = NULL;

    pthread_mutex_lock(&(pool->queue_lock));

    member = pool->queue_head;
    if(NULL != member){
        while (NULL != member->next){
            member = member->next;
        }
        member->next = new_job;
    }else{
        pool->queue_head = new_job;
    }

    if(NULL == pool->queue_head){
        printf("add job error\n");
        pthread_mutex_unlock(&(pool->queue_lock));
        free(new_job);
        return -1;
    }
    pool->waiting_job_num += 1;

    free_thread_num = pool->current_thread_num - pool->working_thread_num;

    if(free_thread_num == 0 && pool->max_free_thread_num > 0){
        pthread_t* new_threadid = (pthread_t*)calloc(pool->current_thread_num+pool->max_free_thread_num, sizeof(pthread_t));
        if(NULL == new_threadid){
            perror("calloc error");
        }else{
            memcpy(new_threadid, pool->threadid, pool->current_thread_num*sizeof(pthread_t));
            for(i = 0; i < pool->max_free_thread_num; ++i){
                pthread_create(&new_threadid[pool->current_thread_num+i], NULL, _threadpool_routine, pool);
            }
            _update_threadpool_max_free(pool);
        }
    }

    pthread_mutex_unlock(&(pool->queue_lock));
    pthread_cond_signal(&(pool->queue_ready));
    return 0;
}