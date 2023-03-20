#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "list.h"
#include "threadpool.h"
#include <stdbool.h>
//static void *work_thread(void *);

struct thread_pool {
    //Array of threads
    pthread_t *threads;
    // number of threads
    int num_threads;
    //strct list_elems
    // array for the local queue, pointer because we need one for each thread?
    struct list job_queue;
    struct list global_queue;
    // condition variable for work thread
    pthread_cond_t cond;
    // pool mutex
    pthread_mutex_t lock;
    // access
    bool access;
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
    //pointer for result job
    void *result;
    void *info;
    struct list_elem elem;
    // 0: not doing anything; 1: working on task; 2: done with tast
    int state;
    struct thread_pool *pool;
    
};

struct worker {
    struct list_elem elem;
    // queue of the worker
    struct list work_queue;
    // track id of the worker thread 
    pthread_t id;
    struct thread_pool *pool;
};

struct thread_pool * thread_pool_new(int nthreads) {
    struct thread_pool *pool = malloc(sizeof(struct thread_pool));
    pool->num_threads = nthreads;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    list_init(&pool->global_queue);
    list_init(&pool->job_queue);
    pool->access = false;

    for (int i = 0; i < nthreads; i++) {
        struct worker *worker = malloc(sizeof(struct worker));

        list_push_front(&pool->job_queue, &worker->elem);
        list_init(&worker->work_queue);
       pthread_create(&worker->id, NULL, work_thread, pool);
    }
    pthread_mutex_unlock(&pool->lock);

    return pool;
}

static void *work_thread(void *arg) {
    struct worker *work = (struct worker*)arg;
    struct thread_pool *pool = work->pool;
    struct list_elem *elem;
    struct list *queue;
    struct future *futre;
    

    while (1) {
        
        pthread_mutex_lock(&pool->lock);

        
        
    }
    return NULL;
}

void thread_pool_shutdown_and_destroy(struct thread_pool *pool) {

    pthread_mutex_lock(&pool->lock);
    pool->access = true;

    if (pthread_cond_broadcast(&pool->cond) != 0) {
        pthread_mutex_unlock(&pool->lock);
    }

    pthread_mutex_unlock(&pool->lock);

    struct list_elem *e;

    for (e = list_begin(&pool->job_queue); e != list_end(&pool->job_queue); e = list_next(e)) {
        struct worker *work = list_entry(e, struct worker, elem);
        pthread_join(work->id, NULL);
        free(work);
    }

    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->lock);
    free(pool);
}