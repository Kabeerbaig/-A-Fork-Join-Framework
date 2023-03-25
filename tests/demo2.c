static void *working_thread(void *param) {
    struct thread_pool *pool = (struct thread_pool *)param;
    struct worker *worker = NULL;

    /* Wait for all worker threads to be created before workers start working */
    pthread_barrier_wait(&pool->start_sync);

    /* Run loop */
    while (1) {
        /* Acquire the lock to access the pool state */
        pthread_mutex_lock(&pool->lock);

        /* Get this worker's info if this is the first iteration */
        if (worker == NULL) {
            worker = get_worker_by_thread_id(pool, pthread_self());
            if (worker == NULL) {
                printf("Error finding worker for thread %lu.\n", pthread_self());
                pool->shutdown = true;
                pthread_mutex_unlock(&pool->lock);
                break;
            }
        }

        /* Wait until there is work to do */
        while (no_pending_work(pool, worker)) {
            #ifdef DEBUG
            printf("No work, now sleeping.\n");
            #endif
            pthread_cond_wait(&pool->work_flag, &pool->lock);
        }

        /* If shutdown flag is set, break out of the loop */
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        /* Get the next task to execute */
        struct future *f = get_next_task(pool, worker);

        /* Release the lock before executing the task */
        pthread_mutex_unlock(&pool->lock);

        /* Execute the task and set the future's result */
        f->result = (f->task)(pool, f->data);

        /* Acquire the lock again to update the future's status */
        pthread_mutex_lock(&pool->lock);
        f->status = COMPLETED;

        /* Notify any thread waiting on the future */
        pthread_cond_signal(&f->done);
        pthread_mutex_unlock(&pool->lock);
    }

    /* Thread is exiting */
    #ifdef DEBUG
    printf("Exiting thread %lu\n", pthread_self());
    #endif

    pthread_exit(NULL);
    return NULL;
}