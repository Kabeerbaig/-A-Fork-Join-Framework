#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <threads.h>
#include "list.h"
#include "threadpool.h"
#include <errno.h>

#include <stdbool.h>
// static void *work_thread(void *);

struct thread_pool
{
    // Array of threads
    pthread_t *threads;
    // number of threads
    int num_threads;
    // strct list_elems
    //  array for the workers
    struct list worker_list;
    // queue for the futures
    struct list global_queue;
    // condition variable for work thread
    pthread_cond_t cond;
    // pool mutex
    pthread_mutex_t lock;
    // shutdown
    bool shutdown;
};
struct future
{
    // future is basially a long runnning task
    // could be complete or not completed
    // long running task
    fork_join_task_t task;
    // This condition will be used to cordinate access to the future
    // When a thread wants access to a future that is not available
    // it will wait on this condition, when results become availaible
    // signal this variable and waiting thread will proceed to execute
    //(Ahmed Yazdani class lecture)
    pthread_cond_t cond;

    // pointer for result job
    void *args;    // data from thread_pool_submit
    void *results; // will store task results once its complete
    struct list_elem elem;
    // 0: not doing anything; 1: working on task; 2: done with task
    int state;
    struct thread_pool *pool;
};

struct worker
{
    struct list_elem elem;
    // queue of the worker
    struct list work_queue;
    // track id of the worker thread
    pthread_mutex_t lock;
    pthread_t id;
    struct thread_pool *pool;
};

/* thread-local worker*/
static thread_local struct worker *local_worker = NULL;

static bool no_pending_work(struct thread_pool *pool, struct worker *worker);
static struct future *get_next_task(struct thread_pool *pool, struct worker *worker);
static void *work_thread(void *arg);
static void *execute_future(struct future *curr_future);

struct thread_pool *thread_pool_new(int nthreads)
{
    struct thread_pool *pool = malloc(sizeof(struct thread_pool));
    pool->num_threads = nthreads;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    list_init(&pool->global_queue);
    list_init(&pool->worker_list);
    pool->shutdown = false;

    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < nthreads; i++)
    {
        struct worker *worker = malloc(sizeof(struct worker));
        worker->pool = pool;
        list_push_front(&pool->worker_list, &worker->elem);
        list_init(&worker->work_queue);
        pthread_mutex_init(&worker->lock, NULL);
        pthread_create(&worker->id, NULL, work_thread, worker);
    }
    pthread_mutex_unlock(&pool->lock);

    return pool;
}

static void *work_thread(void *arg)
{
    struct worker *curr_worker = arg;
    struct thread_pool *pool = curr_worker->pool;

    // set the worker tasks list to differenentiate between global or local submission
    local_worker = curr_worker;

    while (true)
    {
        pthread_mutex_lock(&pool->lock);

        // checks to see if any tasks need to be completed
        // if the queues are empty then run in idle mode
        while (no_pending_work(pool, local_worker))
        {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        // checks shutdown flag
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        // Get the next task to execute
        struct future *curr_future = get_next_task(pool, curr_worker);

        execute_future(curr_future);

        pthread_mutex_unlock(&pool->lock);
    }
    return NULL;
}

void thread_pool_shutdown_and_destroy(struct thread_pool *pool)
{

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    struct list_elem *e = list_begin(&pool->worker_list);

    for (int i = 0; i < pool->num_threads; i++)
    {
        struct worker *curr_worker = list_entry(e, struct worker, elem);

        pthread_join(curr_worker->id, NULL);
        e = list_next(e);
        free(curr_worker);
    }

    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->lock);
    free(pool);
}

// Returns true if there is no pending work in any of the queues, false otherwise.

static bool no_pending_work(struct thread_pool *pool, struct worker *worker)
{

    if (!list_empty(&worker->work_queue))
    {
        return false;
    }

    // checks shutdown falg
    if (pool->shutdown)
    {
        return false;
    }

    // Check if any worker queue has pending work
    struct list_elem *e = list_begin(&pool->worker_list);

    for (int i = 0; i < pool->num_threads; i++)
    {
        struct worker *curr_worker = list_entry(e, struct worker, elem);

        if (!list_empty(&curr_worker->work_queue))
            return false;
    }

    // Check if the global queue has pending work
    if (!list_empty(&pool->global_queue))
    {
        return false;
    }

    return true;
}

/**
 * Returns the next task to be executed, or NULL if there is no pending work.
 * Tasks are taken in a first-come, first-served basis from the worker queues and
 * the global queue, with workers preferring to take from their own queues before
 * the global queue.
 */
static struct future *get_next_task(struct thread_pool *pool, struct worker *worker)
{
    struct list_elem *e;
    struct future *new_future;
    // Check worker's own queue
    if (!list_empty(&worker->work_queue))
    {
        e = list_pop_front(&worker->work_queue);

        new_future = list_entry(e, struct future, elem);

        return new_future;
    }

    // Check global queue
    if (!list_empty(&pool->global_queue))
    {
        e = list_pop_back(&pool->global_queue);
        new_future = list_entry(e, struct future, elem);

        return new_future;
    }

    // Steal from other workers
    struct list_elem *ele;
    struct worker *work;
    for (ele = list_begin(&pool->worker_list); ele != list_end(&pool->worker_list); ele = list_next(ele))
    {
        work = list_entry(ele, struct worker, elem);
        if (work != worker && !list_empty(&work->work_queue))
        {
            e = list_pop_back(&work->work_queue);
            new_future = list_entry(e, struct future, elem);

            return new_future;
        }
    }

    return NULL;
}

struct future *thread_pool_submit(struct thread_pool *pool, fork_join_task_t task, void *data)
{
    /**
     *
     * allocate a new future
     *
     * submit it to the pool
     *
     * need to use a thread local variable to distinguish between an internal and external submission
     */

    // allocate new future
    struct future *new_future = malloc(sizeof(struct future));

    // set task and args for future
    new_future->task = task;
    new_future->args = data;
    new_future->pool = pool;
    new_future->state = 0;

    pthread_cond_init(&new_future->cond, NULL);

    // if thread local variable null, then place future into global queue
    // else, place in local queue
    if (local_worker == NULL)
    {
        pthread_mutex_lock(&pool->lock);

        list_push_back(&pool->global_queue, &new_future->elem);
        pthread_cond_broadcast(&pool->cond);
        pthread_cond_signal(&new_future->cond);

        pthread_mutex_unlock(&pool->lock);
    }
    else
    {
        pthread_mutex_lock(&pool->lock);
        list_push_front(&local_worker->work_queue, &new_future->elem);
        pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
    }

    return new_future;
}

void *future_get(struct future *future_task)
{
    pthread_mutex_lock(&future_task->pool->lock);

    // if the future is already complete then we can just return it
    if (future_task->state == 2)
    {
        pthread_mutex_unlock(&future_task->pool->lock);
        return future_task->results;
    }
    // if internal thread and task not yet started
    // complete the task and return it
    if (local_worker != NULL && future_task->state == 0)
    {
        list_remove(&future_task->elem);
        execute_future(future_task);
        // pthread_cond_signal(&future_task->pool->cond);

        pthread_mutex_unlock(&future_task->pool->lock);

        return future_task->results;
    }

    // else it is an internal thread waiting for the future to complete
    // or the main thread
    while (future_task->state != 2)
    {
        // waiting on future_task conditional and give it the pool lock? or should give it the thread lock
        pthread_cond_wait(&future_task->cond, &future_task->pool->lock);
    }

    pthread_mutex_unlock(&future_task->pool->lock);

    return future_task->results;
}

void future_free(struct future *future_task)
{
    pthread_cond_destroy(&future_task->cond);
    free(future_task);
}

static void *execute_future(struct future *curr_future)
{
    struct thread_pool *pool = curr_future->pool;
    curr_future->state = 1;
    pthread_cond_signal(&curr_future->cond);
    // pthread_cond_signal(&curr_future->pool->cond);

    // execute task and sets future results
    pthread_mutex_unlock(&pool->lock);
    curr_future->results = curr_future->task(pool, curr_future->args);
    // calls pthread submit and future get
    pthread_mutex_lock(&pool->lock);

    curr_future->state = 2; // 2 to mean completed
    pthread_cond_signal(&curr_future->cond);

    return NULL;
}