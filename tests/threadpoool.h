#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "deque.c"
#include "threadpool.h"

#ifndef ThreadPoool_h
#define ThreadPoool_h 

struct ThreadJob {
//void pointers 
void *(*job) (void *arg);
void *arg;

};

struct ThreadPool {
int num_threads; // number of threads there are going to be
int active; // state of the pool, active or inactive
deque_t *deque; 
pthread_t *pool; //an array of unknown size, this pointer will point to the beginning of the array
pthread_mutex_t lock; //only allows one thread to work on it at a time; thread safe
pthread_cond_t signal;

void (*add_work)(struct ThreadPool *thread_pool, struct ThreadJob job);

};

// This data structure will represent jobs in a queue
//each job will take a function and parameter 
//You will be able to put different types of jobs in the same queue


struct ThreadPool thread_poool(int num_threads);
struct ThreadJob thread_job_constructor(void* (*job_function) (void * arg), void *arg);
#endif