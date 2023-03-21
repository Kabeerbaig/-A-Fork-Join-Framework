#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <threads.h>
#include "list.h"
#include "threadpool.h"
#include <stdbool.h>
// static void *work_thread(void *);

/* thread-local worker list*/
static thread_local struct list *worker_tasks_list = NULL;

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
    pthread_t id;
    struct thread_pool *pool;
};

struct thread_pool *thread_pool_new(int nthreads)
{
    struct thread_pool *pool = malloc(sizeof(struct thread_pool));
    pool->num_threads = nthreads;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    list_init(&pool->global_queue);
    list_init(&pool->worker_list);
    pool->shutdown = false;

    for (int i = 0; i < nthreads; i++)
    {
        struct worker *worker = malloc(sizeof(struct worker));

        list_push_front(&pool->worker_list, &worker->elem);
        list_init(&worker->work_queue);
        pthread_create(&worker->id, NULL, work_thread, pool);
    }
    pthread_mutex_unlock(&pool->lock);

    return pool;
}

static void *work_thread(void *arg)
{

    struct thread_pool *pool = (struct ThreadPool *)arg;

    // TODO: how to get the current threads worker information?
    struct worker *worker = NULL;

    // set the worker tasks list to differenentiate between global or local submission
    worker_tasks_list = &worker->work_queue;

    pthread_mutex_lock(&pool->lock);
    while (true)
    {

        // checks to see if any tasks need to be completed
        // if the queues are empty then run in idle mode
        while (no_pending_work(pool))
        {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        // checks shutdown falg
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        // Get the next task to execute
        struct future *futre = get_next_task(pool, worker);

        // execute task and sets future results (got this from the slides)
        futre->results = (futre->task)(pool, futre->results);
        futre->state = 2; // 2 to mean completed
        pthread_cond_signal(&futre->cond);
    }
    pthread_mutex_unlock(&pool->lock);
    return NULL;
}

void thread_pool_shutdown_and_destroy(struct thread_pool *pool)
{

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;

    if (pthread_cond_broadcast(&pool->cond) != 0)
    {
        pthread_mutex_unlock(&pool->lock);
    }

    pthread_mutex_unlock(&pool->lock);

    struct list_elem *e;

    for (e = list_begin(&pool->worker_list); e != list_end(&pool->worker_list); e = list_next(e))
    {
        struct worker *work = list_entry(e, struct worker, elem);
        pthread_join(work->id, NULL);
        free(work);
    }

    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->lock);
    free(pool);
}

// Returns true if there is no pending work in any of the queues, false otherwise.

static bool no_pending_work(struct thread_pool *pool)
{
    struct list_elem *e;
    struct worker *worker;

    // Check if any worker queue has pending work
    for (e = list_begin(&pool->worker_list); e != list_end(&pool->worker_list); e = list_next(e))
    {
        worker = list_entry(e, struct worker, elem);
        if (!list_empty(&worker->work_queue))
        {
            return false;
        }
    }

    // Check if the global queue has pending work
    if (!list_empty(&pool->global_queue))
    {
        return false;
    }

    // TODO: Implement work stealing here

    return true;
}

/**
 * Returns the next task to be executed, or NULL if there is no pending work.
 * Tasks are taken in a first-come, first-served basis from the worker queues and
 * the global queue, with workers preferring to take from their own queues before
 * the global queue.
 */
static struct list_elem *get_next_task(struct thread_pool *pool, struct worker *worker)
{
    struct list_elem *e;

    // Check worker's own queue
    if (!list_empty(&worker->work_queue))
    {
        e = list_pop_front(&worker->work_queue);
        return e;
    }

    // Check global queue
    if (!list_empty(&pool->global_queue))
    {
        e = list_pop_front(&pool->global_queue);
        return e;
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
            return e;
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

    // if thread local variable null, then place future into global queue
    // else, place in local queue
    if (worker_tasks_list == NULL)
    {
        pthread_mutex_lock(&pool->lock);

        list_push_back(&pool->global_queue, &new_future->elem);

        pthread_mutex_unlock(&pool->lock);
    }
    else
    {
        // TODO: How to get the worker info?
        list_push_front()
    }
}

void *future_get(struct future *future_task)
{
    // TODO: How to wait for future task to be completed?
    while (future_task->results != 2)
    {
        // waiting on future_task conditional and give it the pool lock? or should give it the thread lock
        pthread_cond_wait(&future_task->cond, &future_task->pool->lock);
    }

    return future_task->results;
}

void future_free(struct future *future_task)
{
}
