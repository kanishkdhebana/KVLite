#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

const size_t kMaxMessage = 4096 ;

int32_t writeFull(int clientFd, const char* buffer, size_t size) {
    while (size > 0) {
        ssize_t bytesWritten = write(clientFd, buffer, size) ;
        
        if (bytesWritten <= 0) {
            return -1 ; 
        }   
        
        assert((size_t)bytesWritten <= size) ;

        size -= (size_t)bytesWritten ;
        buffer += bytesWritten ;
    }

    return 0 ;
}


int32_t readFull(int clientFd, char* buffer, size_t size) {
    while (size > 0) {
        int bytesRead = read(clientFd, buffer, size) ;

        if (bytesRead <= 0) { 
            return -1 ; 
        } 

        assert((size_t)bytesRead <= size) ;

        size -= (size_t)bytesRead ;
        buffer += bytesRead ;
    }
    
    return 0 ;
}

int singleClientComm(int clientFd) {
    char rbuf[4 + kMaxMessage] ;
    errno = 0 ;

    int err = readFull(clientFd, rbuf, 4) ;
    
    if (err) {
        const char* msg = "EOF" ;

        if (errno != 0) {
            msg = "read() error" ;
        }

        return err ;
    }

    int clientMessageLength = 0 ;
    memcpy(&clientMessageLength, rbuf, 4) ; 
    
    if (clientMessageLength > kMaxMessage) {
        fprintf(stderr, "Message too long: %d bytes\n", clientMessageLength) ;
        return -1 ;
    }

    err = readFull(clientFd, rbuf + 4, clientMessageLength) ;

    if (err) {
        const char* msg = "EOF" ;

        if (errno != 0) {
            msg = "read() error" ;
        }

        return err ;
    }

    printf("Client(%d): %.*s\n", clientMessageLength, clientMessageLength, rbuf + 4) ;

    // reply
    const char reply[] = "Hey buddy, I got your message!" ;
    
    if (strlen(reply) > kMaxMessage) {
        fprintf(stderr, "Reply too long: %zu bytes\n", strlen(reply)) ;
        return -1 ;
    }


    char wbuf[4 + sizeof(reply)] ;
    int replyLength = strlen(reply) ;

    memcpy(wbuf, &replyLength, 4) ;
    memcpy(wbuf + 4, reply, replyLength) ; 
    
    err = writeFull(clientFd, wbuf, 4 + replyLength) ;
    
    if (err) {
        perror("writeFull") ;
        return -1 ;
    }
    
    else {
        printf("Server(%d): %.*s\n", 4 + replyLength, (int)replyLength, reply) ;
    }

    return 0 ;
}


int main() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0) ;
    
    if (serverFd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    } 
    
    else {
        printf("Socket created successfully with file descriptor: %d\n", serverFd) ;
    }

    struct sockaddr_in serverAddress = {} ;
    serverAddress.sin_family = AF_INET ;
    serverAddress.sin_port = htons(1234) ; 
    serverAddress.sin_addr.s_addr = INADDR_ANY ; 

    int bindResult = bind(serverFd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) ;
    
    if (bindResult < 0) {
        perror("bind") ;

        close(serverFd) ;
        exit(EXIT_FAILURE) ;
    } 
    
    else {
        printf("Socket bound successfully to port 1234\n") ;    
    }

    int listenResult = listen(serverFd, 2) ;
    
    if (listenResult < 0) {
        perror("listen") ;
        close(serverFd) ;  
        exit(EXIT_FAILURE) ;
    }

    while (true) {
        struct sockaddr_in clientAddress = {} ;
        socklen_t clientAddressLength = sizeof(clientAddress) ;
        int clientFd = accept(serverFd, (struct sockaddr*) &clientAddress, &clientAddressLength) ;
        
        if (clientFd < 0) {
            continue ;
        } 

        else {
            printf("Accepted connection from client with file descriptor: %d\n", clientFd) ;
        }

        while (true) {
            int err = singleClientComm(clientFd) ;

            if (err) {
                printf("Some problem with connection, exiting.\n") ;
                break ;
            }
        }
        
        close(clientFd) ;
    }
}