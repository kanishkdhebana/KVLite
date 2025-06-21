#include "handler.h"
#include "connection.h"
#include "util.h"

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

    printf("Server is listening on port 1234\n") ;
    

    // Set the socket to non-blocking mode
    fdSetNonBlocking(serverFd) ;

    int listenResult = listen(serverFd, 12000) ;
    
    if (listenResult < 0) {
        die("listen") ;
    }

    std::vector<Connection*> connections ;  
    std::vector<struct pollfd> pollFds ;

    while (true) {
        pollFds.clear() ;

        struct pollfd pfd = {serverFd, POLLIN, 0} ;
        pollFds.push_back(pfd) ;

        for (Connection* conn: connections) {
            if (!conn) {
                continue ;
            }

            struct pollfd pfd = {conn -> connectionFd, POLLERR, 0} ;

            if (conn -> wantToRead) {
                pfd.events |= POLLIN ;
            }

            if (conn -> wantToWrite) {
                pfd.events |= POLLOUT ;
            }

            pollFds.push_back(pfd) ;
        }

        int pollResult = poll(pollFds.data(), pollFds.size(), -1) ;

        if (pollResult < 0 && errno == EINTR) {
            continue ;
        }

        else if (pollResult < 0) {
            die("poll") ;
        }

        if (pollFds[0].revents) {
            Connection* newConnection = handleAccept(serverFd) ;

            if (newConnection) {
                if (connections.size() <= (size_t)newConnection -> connectionFd) {
                    connections.resize(newConnection -> connectionFd + 1, nullptr) ;
                }

                assert(!connections[newConnection -> connectionFd]) ;
                connections[newConnection -> connectionFd] = newConnection ;
            }
        }


        for (size_t i = 1; i < pollFds.size(); i++) { // skip the server socket
            struct pollfd pfd = pollFds[i] ;
            uint32_t ready = pfd.revents ;
            Connection* conn = connections[pfd.fd] ;

            if (ready & POLLIN) {
                assert(conn -> wantToRead) ;
                handleRead(conn) ;
            }

            if (ready & POLLOUT) {
                assert(conn -> wantToWrite) ;
                handleWrite(conn) ;
            }

            if (ready & POLLERR || conn -> wantToClose) {
                (void) close(conn -> connectionFd) ;
                connections[conn -> connectionFd] = nullptr ;   
                delete conn ;
            }
        }
    }
}