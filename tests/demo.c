/**
 * Returns the worker struct corresponding to the given thread ID.
 * If no worker with the given ID is found, returns NULL.
 */
static struct worker *get_worker_by_thread_id(struct thread_pool *pool, pthread_t tid) {
    struct list_elem *e;
    struct worker *w;
    for (e = list_begin(&pool->worker_threads); e != list_end(&pool->worker_threads); e = list_next(e)) {
        w = list_entry(e, struct worker, elem);
        if (pthread_equal(w->tid, tid)) {
            return w;
        }
    }
    return NULL;
}

/**
 * Returns true if there is no pending work in any of the queues, false otherwise.
 */
static bool no_pending_work(struct thread_pool *pool) {
    struct list_elem *e;
    struct worker *w;

    // Check if any worker queue has pending work
    for (e = list_begin(&pool->worker_threads); e != list_end(&pool->worker_threads); e = list_next(e)) {
        w = list_entry(e, struct worker, elem);
        if (!list_empty(&w->worker_queue)) {
            return false;
        }
    }

    // Check if the global queue has pending work
    if (!list_empty(&pool->global_queue)) {
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
static struct list_elem *get_next_task(struct thread_pool *pool, struct worker *w) {
    struct list_elem *e;

    // Check worker's own queue
    if (!list_empty(&w->worker_queue)) {
        e = list_pop_front(&w->worker_queue);
        return e;
    }

    // Check global queue
    if (!list_empty(&pool->global_queue)) {
        e = list_pop_front(&pool->global_queue);
        return e;
    }

    // Steal from other workers
    struct list_elem *we;
    struct worker *wt;
    for (we = list_begin(&pool->worker_threads); we != list_end(&pool->worker_threads); we = list_next(we)) {
        wt = list_entry(we, struct worker, elem);
        if (wt != w && !list_empty(&wt->worker_queue)) {
            e = list_pop_back(&wt->worker_queue);
            return e;
        }
    }

    return NULL;
}