#include "time.h"
#include "util.h"
#include "heap.h"
#include "datastore.h"
#include "connection.h"

#include <ctime>
#include <cstdint>
#include <cstdio>
#include <cassert>

const uint64_t kIdleTimeoutMS = 5 * 1000 ;


uint64_t getMonoticMS() {
    timespec tv = {0, 0} ;
    clock_gettime(CLOCK_MONOTONIC, &tv) ;    

    return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000 ;
}


int32_t nextTimerMS() {
    uint64_t nowMS = getMonoticMS() ;;
    uint64_t nextMS = (uint64_t)-1 ;

    if (!dlistEmpty(&g_data.idleList)) {
        Connection* conn = container_of(g_data.idleList.next, Connection, idleNode) ;
        nextMS = (conn -> lastActiveMS) + kIdleTimeoutMS ;
    }

    if (!g_data.heap.empty() && g_data.heap[0].val < nextMS) {
        nextMS = g_data.heap[0].val ;
    }

    if (nextMS == (uint64_t)-1 ) {
        return -1 ;
    }

    if (nextMS <= nowMS) {
        return 0 ;
    }

    return (int32_t)(nextMS - nowMS) ;
}


static void entryDeleteSync(Entry* entry) {
    if (entry -> type == T_ZSET) {
        zsetClear(&entry -> zset) ;
    }

    delete entry ;
}

static void entryDeleteFun(void* arg) {
    entryDeleteSync((Entry*)arg) ;
}

static void entryDelete(Entry* entry) {
    entrySetTTL(entry, -1) ;
    
    size_t setSize = (entry -> type == T_ZSET)? hmSize(&entry -> zset.hmap) : 0 ;
    const size_t kLargeContainerSize = 1000 ;

    if (setSize > kLargeContainerSize) {
        threadPoolQueue(&g_data.thread_pool, &entryDeleteFun, entry) ;
    }

    else {
        entryDeleteSync(entry) ;
    }
}

static bool hnodeSame(const HNode *node, const HNode *key) {
    return node == key ;
}

void processTimers() {
    uint64_t nowMS = getMonoticMS() ;

    // printf(
    //     "Hello from processTimers\n"
    // ) ;

    // printf(
    //     "Current time: %llu ms\n", nowMS
    // ) ;

    // printf("is there any connection in the idleLIst"
    // ) ;
    // if (dlistEmpty(&g_data.idleList)) {
    //     printf("No idle connections\n") ;
    // } else {
    //     printf("Yes, there are idle connections\n") ;
    // }


    while (!dlistEmpty(&g_data.idleList)) {
        Connection* conn = container_of(g_data.idleList.next, Connection, idleNode) ;
        uint64_t nextMS = conn -> lastActiveMS + kIdleTimeoutMS ;

        // printf("last active ms: %lld\n", conn -> lastActiveMS) ;
        // printf("timeout time: %lld\n", nextMS) ;

        if (nextMS >= nowMS) {
            break ; 
        }

        fprintf(stderr, "removing idle connection: %d\n", conn -> connectionFd) ;
        connDestroy(conn) ;
    }

    std::vector<HeapItem>& heap = g_data.heap ;
    const size_t kMaxWorks = 2000 ;
    size_t nWorks = 0 ;

    while (!heap.empty() && heap[0].val < nowMS) {
        Entry* entry = container_of(heap[0].ref, Entry, heapIdx) ;
        HNode* node = hmDelete(&g_data.db, &entry -> node, &hnodeSame) ;
        assert(node == &entry -> node) ;

        entryDelete(entry) ;

        if (nWorks++ >= kMaxWorks) {
            break;
        }
    }
}

