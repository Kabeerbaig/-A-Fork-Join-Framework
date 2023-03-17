#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "deque.c"
#include "threadpool.h"
#include "threadpoool.h"
#include "deque.c"

void *thread_job(void *arg);
void add_work(struct ThreadPool *thread_pool, struct ThreadJob job);
struct ThreadPool thread_poool(int num_threads) {

    struct ThreadPool thread_pool;
    thread_pool.num_threads = num_threads;
    thread_pool.active = 1;
    // this will create the threads using malloc
    thread_pool.pool = (pthread_t *)malloc(sizeof(pthread_t[num_threads]));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&thread_pool.pool[i], NULL,thread_job,NULL);
    }
    thread_pool.deque = create_deque(); // double check this; you passed a pointer in the ThreadPool struct
    thread_pool.lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    thread_pool.signal = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    thread_pool.add_work = add_work;
    return thread_pool;
}

void thread_pool_destory(struct ThreadPool * thread_pool) {

    thread_pool->active = 0;
    for (int i = 0; i < thread_pool->num_threads; i++) {
        pthread_cond_signal(&thread_pool->signal);
    }
    for (int j = 0; j < thread_pool->num_threads; j++) {
        pthread_join(thread_pool->pool[j], NULL);
    }

    free(thread_pool->pool);
    free_deque(&thread_pool->deque);
}

struct ThreadJob thread_job_constructor(void* (*job_function) (void * arg), void *arg) {
    struct ThreadJob job;
    job.job = job_function;
    job.arg = arg;
    return job;
}
// take something out of the queue
//execute the job that is in the queue

void * thread_job(void *arg) {
    // put it into an infinite loop; waiting for something to come into the queue
    //continue waiting until pool is no longer active
    // cast the arg as a threadpool so you can reference it 

    struct ThreadPool *thread_pool = (struct ThreadPool *) arg;
    while (thread_pool->active == 1) {
        // pull a threadsafe job out of queue
        pthread_mutex_lock(&thread_pool->lock);
        // send signal to indicate there is something in deque, allows one thread to come through
        pthread_cond_wait(&thread_pool->signal, &thread_pool->lock);
        struct ThreadJob job;
        if (!is_empty(thread_pool->deque)) {
            job = * (struct ThreadJob*)front_element((&thread_pool->deque)); // get the first thing in the deque
        } 
        remove_front(&thread_pool->deque);
        pthread_mutex_unlock(&thread_pool->lock);
       if (job.job) {
        job.job(job.arg);
       }

    }
    retrun NULL;
}
void add_work(struct ThreadPool *thread_pool, struct ThreadJob job) {
    push_back(&thread_pool->deque, &job);
}