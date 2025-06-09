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

    
    const char* message = "Hello, Server!" ;
    ssize_t bytesWritten = write(fd, message, strlen(message)) ;
    
    if (bytesWritten < 0) {
        perror("write") ;
        close(fd) ;
        exit(EXIT_FAILURE) ;    
    }
    
    else {
        printf("Wrote %zd bytes to the socket\n", bytesWritten) ;
    }

    char readBuffer[1024] = {} ;
    ssize_t bytesRead = read(fd, readBuffer, sizeof(readBuffer) - 1) ;
    
    if (bytesRead < 0) {        
        perror("read") ;
        close(fd) ;
        exit(EXIT_FAILURE) ;
    } 
    
    else {
        readBuffer[bytesRead] = '\0' ;
        printf("Read %zd bytes: %s\n", bytesRead, readBuffer) ;
    }

    close(fd) ;
    return 0 ;
}