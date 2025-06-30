#include "thread_pool.h"

#include <cassert>

static void* worker(void* arg) {
    ThreadPool* tp = (ThreadPool*)arg ;

    while (true) {
        pthread_mutex_lock(&tp -> mutex) ;

        while (tp -> queue.empty()) {
            pthread_cond_wait(&tp -> notEmpty, &tp -> mutex) ;
        }

        Work w = tp -> queue.front() ;
        tp -> queue.pop_front() ;
        pthread_mutex_unlock(&tp -> mutex) ;

        w.f(w.arg) ;

        return NULL ;
    }
}

void threadPoolInit(ThreadPool* tp, size_t numThreads) {
    assert(numThreads > 0) ;

    int rv = pthread_mutex_init(&tp -> mutex, NULL) ;
    assert(rv == 0) ;

    rv = pthread_cond_init(&tp -> notEmpty, NULL) ;
    assert(rv == 0) ;

    tp -> threads.resize(numThreads) ;

    for (size_t i = 0; i < numThreads; i++) {
        int rv = pthread_create(&tp -> threads[i], NULL, &worker, tp) ;
        assert(rv == 0) ;
    }
}


void threadPoolQueue(ThreadPool* tp, void (*f)(void*), void* arg) {
    pthread_mutex_lock(&tp -> mutex) ;
    tp -> queue.push_back(Work {f, arg}) ;
    pthread_cond_signal(&tp -> notEmpty) ;
    pthread_mutex_unlock(&tp -> mutex) ;
}