#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "list.h"
#include "threadpool.h"
struct thread_pool {
    //threads
    pthread_t *threads;
    // number of threads
    int num_threads;
    //strct list_elems
    struct list w_threads;
    struct list global_queue;
    // condition variables 
    pthread_cond_t queue_cond;
    // pool mutex
    pthread_mutex_t lock;
    // access
    int access;
};
struct future {
    // future is basially a long runnning task
    // could be complete or not completed 
    // long running task 
    fork_join_task_t task;
    // This condition will be used to cordinate access to the future
    //When a thread wants access to a future that is not available 
    // it will wait on this condition, when results become availaible
    //signal this variable and waiting thread will proceed to execute
    //(Ahmed Yazdani class lecture)
    pthread_cond_t cond;
    // when the future task is complete
    int completed;
    //manages the future 
    struct thread_pool *f_pools;
};

struct worker {
    struct list_elem elem;
    // queue of the worker
    struct list queue;
    // track id of the worker thread 
    pthread_t id;
};
struct thread_pool * thread_pool_new(int nthreads) {
    struct thread_pool *pool = malloc(sizeof(struct thread_pool));

    if (pool == NULL) {
        return NULL;
    }

    pool->num_threads = nthreads;
    pool->threads = malloc(nthreads * sizeof(pthread_t));
    list_init(&pool->global_queue); 
    list_init(&pool->w_threads);
    pool->access = 0;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);


    pool->access = 1;

    return pool;
};