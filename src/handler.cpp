#include "handler.h"
#include "connection.h"
#include "buffer.h"
#include "util.h"
#include "protocol.h"

#include <cstdio>     
#include <cassert>    
#include <cerrno>    
#include <unistd.h>  
#include <fcntl.h>   
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


Connection* handleAccept(int serverFd) {

    struct sockaddr_in clientAddress = {} ;
    socklen_t clientAddressLength = sizeof(clientAddress) ;
    
    int connectionFd = accept(serverFd, (struct sockaddr*) &clientAddress, &clientAddressLength) ;
    
    if (connectionFd < 0) {
        perror("accept") ;
        return nullptr ;
    }   

    uint32_t ip = clientAddress.sin_addr.s_addr;

    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
        ntohs(clientAddress.sin_port)
    );

    fdSetNonBlocking(connectionFd) ;

    return new Connection(connectionFd) ;
}


void handleWrite(Connection * conn) {
    assert(conn -> wantToWrite) ;

    ssize_t bytesWritten = write(
        conn -> connectionFd, conn -> writeBuffer -> dataStart, conn -> writeBuffer -> size()
    ) ;

    if (bytesWritten < 0 && errno == EAGAIN) {
        return ;
    }

    if (bytesWritten < 0) {
        perror("write") ;
        conn -> wantToClose = true ;
        return ;
    }

    conn -> writeBuffer -> consume(bytesWritten) ;

    if (conn -> writeBuffer -> size() == 0) {
        conn -> wantToWrite = false ;
        conn -> wantToRead = true ;
    } 
}


void handleRead(Connection * conn) {
    uint8_t buffer[64 * 1024] ;
    ssize_t bytesRead = read(conn -> connectionFd, buffer, sizeof(buffer)) ;

    if (bytesRead < 0 && errno == EAGAIN) {
        return ;
    }

    if (bytesRead < 0) {
        perror("read") ;
        conn -> wantToClose = true ;
        return ;
    } 

    if (bytesRead == 0) {
        if (conn -> readBuffer -> size() == 0) {
            perror("client closed") ;
        } 
        
        else {
            perror("unexpected EOF") ;
        }
        
        conn -> wantToClose = true ;
        return ;
    }

    conn -> readBuffer -> append(buffer, bytesRead) ;


    while (tryOneRequest(conn)) {   
        // Successfully parsed a request, continue to the next one
    }
    

    if (conn -> writeBuffer -> size() > 0) {
        conn -> wantToWrite = true ;
        conn -> wantToRead = false ;

        return handleWrite(conn) ;
    }
}