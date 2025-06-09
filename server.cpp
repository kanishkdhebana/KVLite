#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0) ;
    
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    } 
    
    else {
        printf("Socket created successfully with file descriptor: %d\n", fd) ;
    }

    struct sockaddr_in serverAddress = {} ;
    serverAddress.sin_family = AF_INET ;
    serverAddress.sin_port = htons(1234) ; 
    serverAddress.sin_addr.s_addr = INADDR_ANY ; 

    int bindResult = bind(fd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) ;
    
    if (bindResult < 0) {
        perror("bind") ;
        close(fd) ;
        exit(EXIT_FAILURE) ;
    } 
    
    else {
        printf("Socket bound successfully to port 1234\n") ;    
    }

    int listenResult = listen(fd, 5) ;
    
    if (listenResult < 0) {
        perror("listen") ;
        close(fd) ;  
        exit(EXIT_FAILURE) ;
    }

    while (true) {
        struct sockaddr_in clientAddress = {} ;
        socklen_t clientAddressLength = sizeof(clientAddress) ;
        int clientFd = accept(fd, (struct sockaddr*) &clientAddress, &clientAddressLength) ;
        
        if (clientFd < 0) {
            perror("accept") ;
            close(fd) ;
            exit(EXIT_FAILURE) ;
        } 

        else {
            printf("Accepted connection from client with file descriptor: %d\n", clientFd) ;
        }
        
        char readBuffer[1024] = {} ;
        ssize_t bytesRead = read(clientFd, readBuffer, sizeof(readBuffer) - 1) ;
        if (bytesRead < 0) {
            perror("read") ;
            close(clientFd) ;
            continue;
        }

        printf("Received message: %.*s\n", (int)bytesRead, readBuffer) ;

        char writeBuffer[1024] = "Hello from server!" ;
        ssize_t bytesWritten = write(clientFd, writeBuffer, strlen(writeBuffer)) ;
        if (bytesWritten < 0) {
            perror("write") ;
            close(clientFd) ;       
            continue;
        }

         close(clientFd) ;
    }
}