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


int32_t query(int fd, const char* message) {
    uint32_t messageLength = (uint32_t)strlen(message) ;

    if (messageLength > kMaxMessage) {
        fprintf(stderr, "Message too long: %d bytes\n", messageLength) ;
        return -1 ;
    }
    
    // ___________later -> try dynamic allocation
    char wbuf[4 + kMaxMessage] ;
    memcpy(wbuf, &messageLength, 4) ; 
    memcpy(wbuf + 4, message, messageLength) ;

    int err = writeFull(fd, wbuf, 4 + messageLength) ;

    if (err) {
        perror("writeFull") ;
        return -1 ;
    }

    else {
        printf("Client(size = %d): %.*s\n", 4 + messageLength, (int)messageLength, message) ;
    }

    char rbuf[4 + kMaxMessage] ;
    err = readFull(fd, rbuf, 4) ;
    
    if (err) {
        const char* msg = "EOF" ;

        if (errno != 0) {
            msg = "read() error" ;
        }

        fprintf(stderr, "%s\n", msg) ;
        return err ;
    }

    int responseLength = 0 ;
    memcpy(&responseLength, rbuf, 4) ; 
    
    if (responseLength > kMaxMessage) {
        fprintf(stderr, "Response too long: %d bytes\n", responseLength) ;
        return -1 ;
    }

    err = readFull(fd, rbuf + 4, responseLength) ;
    
    if (err) {
        const char* msg = "EOF" ;

        if (errno != 0) {
            msg = "read() error" ;
        }

        fprintf(stderr, "%s\n", msg) ;
        return err ;
    }

    printf("Server(size = %d): %.*s\n", responseLength, responseLength, rbuf + 4) ;
    return 0 ;
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0) ;
    
    if (fd < 0) {
        perror("socket") ;
        exit(EXIT_FAILURE) ;
    } 
    
    else {
        printf("Socket created successfully with file descriptor: %d\n", fd) ;
    }

    struct sockaddr_in serverAddress = {} ;
    serverAddress.sin_family = AF_INET ;
    serverAddress.sin_port = htons(1234) ;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); ; 

    int connectionResult = connect(fd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) ;

    if (connectionResult < 0) {
        perror("connect");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    else {
        printf("Connected to server successfully\n") ;
    }


    int32_t err = query(fd, "kanishk");
    if (err) {
        printf("Error in query 1: %d\n", err);
        goto L_DONE;
    }

    err = query(fd, "hello2");

    if (err) {
        goto L_DONE;
    }

    err = query(fd, "hello3");
    
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}