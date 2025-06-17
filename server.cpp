#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <poll.h>
#include <fcntl.h>

const size_t kMaxMessage = 32 << 20 ;

struct Connection {
    int connectionFd = -1 ;
    bool wantToRead = false ;
    bool wantToWrite = false ;
    bool wantToClose = false ;

    std::vector<uint8_t> readBuffer ;
    std::vector<uint8_t> writeBuffer ;

    Connection(int fd) : connectionFd(fd), wantToRead(true), wantToWrite(false), wantToClose(false) {}
} ;


static void die(const char* message) {
    fprintf(stderr, "[%d] %s\n", errno, message) ;
    abort() ;
}

static void fdSetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0) ;
    
    if (flags < 0) {
        perror("fcntl(F_GETFL)") ;
        exit(EXIT_FAILURE) ;
    }

    flags |= O_NONBLOCK ;

    if (fcntl(fd, F_SETFL, flags) < 0) {
        perror("fcntl(F_SETFL)") ;
        exit(EXIT_FAILURE) ;
    }
}

static void bufferAppend(std::vector<uint8_t>& buffer, const uint8_t* data, size_t size) {
    if (size == 0) {
        return ;
    }

    size_t oldSize = buffer.size() ;
    buffer.resize(oldSize + size) ;
    memcpy(buffer.data() + oldSize, data, size) ;
}

static void bufferConsume(std::vector<uint8_t>& buffer, size_t size) {
    if (size == 0 || size > buffer.size()) {
        return ;
    }

    size_t newSize = buffer.size() - size ;
    
    if (newSize > 0) {
        memmove(buffer.data(), buffer.data() + size, newSize) ;
    }

    buffer.resize(newSize) ;
}

static bool tryOneMessage(Connection* conn) {
    if (conn -> readBuffer.size() < 4) {
        return false ; // not enough data to read the message length
    }

    int32_t messageLength = 0 ;
    memcpy(&messageLength, conn -> readBuffer.data(), 4) ;

    if (messageLength < 0 || (size_t)messageLength > kMaxMessage) {
        fprintf(stderr, "Invalid message length: %d\n", messageLength) ;
        conn -> wantToClose = true ;
        return false ;
    }

    if (conn -> readBuffer.size() < (size_t)(4 + messageLength)) {
        return false ; // not enough data to read the full message
    }

    const uint8_t* messageData = conn -> readBuffer.data() + 4 ;

    bufferAppend(conn -> writeBuffer, (const uint8_t*)& messageLength, 4) ;
    bufferAppend(conn -> writeBuffer, messageData, messageLength) ;

    
    
    bufferConsume(conn -> readBuffer, 4 + messageLength) ;

    printf("Client: ") ;
    for (size_t i = 0; i < messageLength; i++) {
        if (messageData[i] >= 32 && messageData[i] <= 126) {
            printf("%c", messageData[i]) ;
        } 
        
        else {
            printf(".") ; 
        }
    }
    printf("\n") ;

    printf("Server:") ;
    for (size_t i = 0; i < messageLength; i++) {
        if (messageData[i] >= 32 && messageData[i] <= 126) {
            printf("%c", messageData[i]) ;
        } 
        
        else {
            printf(".") ; 
        }
    }
    printf("\n") ;

    return true ;
}

Connection* handleAccept(int serverFd) {
    struct sockaddr_in clientAddress = {} ;
    socklen_t clientAddressLength = sizeof(clientAddress) ;
    
    int connectionFd = accept(serverFd, (struct sockaddr*) &clientAddress, &clientAddressLength) ;
    
    if (connectionFd < 0) {
        perror("accept") ;
        return nullptr ;
    }

    printf("Accepted connection from %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port)) ;

    fdSetNonBlocking(connectionFd) ;

    return new Connection(connectionFd) ;
}


void handleRead(Connection * conn) {
    uint8_t buffer[64 * 1024] ;
    ssize_t bytesRead = read(conn -> connectionFd, buffer, sizeof(buffer)) ;

    if (bytesRead <= 0) {
        conn -> wantToClose = true ;
        return ;
    } 

    bufferAppend(conn -> readBuffer, buffer, bytesRead) ;
    tryOneMessage(conn) ;

    

    if (conn -> writeBuffer.size() > 0) {
        conn -> wantToWrite = true ;
        conn -> wantToRead = false ;
    } else {
        conn -> wantToRead = true ;
        conn -> wantToWrite = false ;
    }
}

void handleWrite(Connection * conn) {
    assert(conn -> wantToWrite) ;
    
    ssize_t bytesWritten = write(conn -> connectionFd, conn -> writeBuffer.data(), conn -> writeBuffer.size()) ;
    
    if (bytesWritten < 0) {
        perror("write") ;
        conn -> wantToClose = true ;
        return ;
    }

    bufferConsume(conn -> writeBuffer, bytesWritten) ;

    if (conn -> writeBuffer.size() == 0) {
        conn -> wantToWrite = false ;
        conn -> wantToRead = true ; // switch back to reading
    } else {
        conn -> wantToWrite = true ; // still have data to write
    }
}



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

    int listenResult = listen(serverFd, 2) ;
    
    if (listenResult < 0) {
        die("listen") ;
    }

    std::vector<Connection*> connections ;  
    std::vector<struct pollfd> pollArguments ;

    while (true) {
        pollArguments.clear() ;

        struct pollfd pfd = {serverFd, POLLIN, 0} ;
        pollArguments.push_back(pfd) ;

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

            pollArguments.push_back(pfd) ;
        }

        int pollResult = poll(pollArguments.data(), pollArguments.size(), -1) ;

        if (pollResult < 0 && errno == EINTR) {
            continue ;
        }

        else if (pollResult < 0) {
            die("poll") ;
        }

        if (pollArguments[0].revents) {
            Connection* newConnection = handleAccept(serverFd) ;

            if (newConnection) {
                if (connections.size() <= (size_t)newConnection -> connectionFd) {
                    connections.resize(newConnection -> connectionFd + 1, nullptr) ;
                }

                assert(!connections[newConnection -> connectionFd]) ;
                connections[newConnection -> connectionFd] = newConnection ;
            }
        }


        for (size_t i = 1; i < pollArguments.size(); i++) { // skip the server socket
            struct pollfd pfd = pollArguments[i] ;
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