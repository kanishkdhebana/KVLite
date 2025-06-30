#include "time.h"
#include "util.h"
#include "datastore.h"
#include "connection.h"

#include <ctime>
#include <cstdint>
#include <cstdio>

const uint64_t kIdleTimeoutMS = 5 * 1000 ;

uint64_t getMonoticMS() {
    timespec tv = {0, 0} ;
    clock_gettime(CLOCK_MONOTONIC, &tv) ;    

    return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000 ;
}

int32_t nextTimerMS() {
    if (dlistEmpty(&g_data.idleList)) {
        return -1 ;
    }

    uint64_t nowMS = getMonoticMS() ;
    Connection* conn = container_of(g_data.idleList.next, Connection, idleNode) ;

    if (!conn) {
        return -1;  
    }

    uint64_t nextMS = (conn -> lastActiveMS) + kIdleTimeoutMS ;

    if (nextMS < nowMS) {
        return 0 ;
    }

    return (int32_t)(nextMS - nowMS) ;
}

void processTimers() {
    uint64_t nowMS = getMonoticMS() ;

    while (!dlistEmpty(&g_data.idleList)) {
        Connection* conn = container_of(g_data.idleList.next, Connection, idleNode) ;
        uint64_t nextMS = conn -> lastActiveMS + kIdleTimeoutMS ;

        if (nextMS >= nowMS) {
            break ; 
        }

        fprintf(stderr, "removing idle connection: %d\n", conn -> connectionFd) ;
        connDestroy(conn) ;
    }
}

