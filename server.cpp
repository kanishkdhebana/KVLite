#include "handler.h"
#include "connection.h"
#include "util.h"
#include "hashtable.h"
#include "datastore.h"
#include "avl.h"
#include "helper_time.h"
#include "log.h"
#include "heap.h"

#include <cstdio>
#include <cstdlib>   
#include <cerrno>    
#include <vector>    
#include <cassert>  
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int main() {
    initLog() ;

    threadPoolInit(&g_data.thread_pool, 4);
    dlistInit(&g_data.idleList) ;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0) ;
    
    if (serverFd < 0) {
        die("socket") ;
    } 

    int val = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) ;

    struct sockaddr_in serverAddress = {} ;
    serverAddress.sin_family = AF_INET ;
    serverAddress.sin_port = htons(1234) ; 
    serverAddress.sin_addr.s_addr = INADDR_ANY ; 

    int bindResult = bind(serverFd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) ;
    
    if (bindResult < 0) {
        die("bind") ;
    } 

    logMessage("Server is listening on port 1234\n");
    
    // Set the socket to non-blocking mode
    fdSetNonBlocking(serverFd) ;

    int listenResult = listen(serverFd, 12000) ;
    
    if (listenResult < 0) {
        die("listen") ;
    }

    std::vector<struct pollfd> pollFds ;

    while (true) {
        pollFds.clear() ;

        // add server to the poll list
        struct pollfd pfd = {serverFd, POLLIN, 0} ;
        pollFds.push_back(pfd) ;

        // add connected clients to the poll list
        for (Connection* conn: g_data.fd2conn) {
            if (!conn) {
                continue ;
            }

            struct pollfd pfd = {conn -> connectionFd, 0, 0} ;

            if (conn -> wantToRead) {
                pfd.events |= POLLIN ;
            }

            if (conn -> wantToWrite) {
                pfd.events |= POLLOUT ;
            }

            pollFds.push_back(pfd) ;
        }

        int32_t timeoutMS = nextTimerMS() ;
        int pollResult = poll(pollFds.data(), pollFds.size(), timeoutMS) ;

        if (pollResult < 0 && errno == EINTR) { // try again next time
            continue ;
        }

        else if (pollResult < 0) {
            die("poll") ;
        }

        // handle server
        if (pollFds[0].revents) {       // pollFds[0] is the server, so just checking for new connection requests.
            handleAccept(serverFd) ;
        }

        // handle clients
        for (size_t i = 1; i < pollFds.size(); i++) { // skip the server socket
            struct pollfd pfd = pollFds[i] ;
            uint32_t ready = pfd.revents ;

            Connection* conn = g_data.fd2conn[pfd.fd] ;

            conn -> lastActiveMS = getMonoticMS() ;
            dlistDetach(&conn -> idleNode) ;
            dlistInsertBefore(&g_data.idleList, &conn -> idleNode) ;

            if (ready & POLLIN) {
                assert(conn -> wantToRead) ;
                handleRead(conn) ;
            }

            if (ready & POLLOUT) {
                assert(conn -> wantToWrite) ;
                handleWrite(conn) ;
            }

            if (ready & POLLERR || conn -> wantToClose) {
                connDestroy(conn) ;
            }
        } // for each connection

        processTimers() ;
    } // event loop

    closeLog() ;
}