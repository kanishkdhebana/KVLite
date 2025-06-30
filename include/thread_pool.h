#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>
#include <pthread.h>
#include <vector>
#include <deque>


struct Work {
    void (*f)(void*) = NULL ;
    void* arg = NULL ;
} ;


struct ThreadPool {
    std::vector<pthread_t> threads ;
    std::deque<Work> queue ;
    pthread_mutex_t mutex ;
    pthread_cond_t notEmpty ;
} ;

void threadPoolInit(ThreadPool* tp, size_t numThreads) ;
void threadPoolQueue(ThreadPool* tp, void (*f)(void*), void* arg) ;

#endif