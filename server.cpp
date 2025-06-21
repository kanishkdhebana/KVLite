#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <string>
#include <vector>
#include <map>


const size_t kMaxMessage = 32 << 20 ;
const size_t kMaxArgs = 200 * 1000;

// placeholder; implemented later
static std::map<std::string, std::string> g_data ;

enum {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

struct Response {
    uint32_t status = 0 ;
    std::vector<uint8_t> data ;
} ;

struct Buffer {
    uint8_t* bufferBegin ;
    uint8_t* bufferEnd ;
    uint8_t* dataStart ;
    uint8_t* dataEnd ;

    Buffer(size_t size) {
        bufferBegin = (uint8_t*)malloc(size) ;

        if (!bufferBegin) {
            perror("malloc") ;
            exit(EXIT_FAILURE) ;
        }

        bufferEnd = bufferBegin + size ;
        dataStart = bufferBegin ;
        dataEnd = bufferBegin ;
    }
    
    ~Buffer() {
        free(bufferBegin) ;
    }

    void append(
        const uint8_t* data, 
        size_t size
    ) {
        size_t availableSpace = (bufferEnd) - (dataEnd) ;

        if (availableSpace < size) {
            size_t currentSize = (bufferEnd) - (bufferBegin) ;
            size_t newSize = (currentSize * 2 > currentSize + size) ? currentSize * 2 : currentSize + size ;

            uint8_t* newBuffer = (uint8_t*)realloc(bufferBegin, newSize);

            if (!newBuffer) {
                perror("realloc") ;
                exit(EXIT_FAILURE) ;
            }

            dataStart = newBuffer + ((dataStart) - (bufferBegin)) ;
            dataEnd = newBuffer + ((dataEnd) - (bufferBegin)) ;
            bufferBegin = newBuffer ;
            bufferEnd = newBuffer + newSize ;
        }

        memcpy(dataEnd, data, size) ;
        dataEnd += size ;
    }

    void consume(  
        size_t size
    ) {
        dataStart += size ;

        if (dataStart == dataEnd) {
            dataStart = bufferBegin ;
            dataEnd = bufferBegin ;
        } 
        
        else if (dataStart > dataEnd) {
            fprintf(stderr, "Buffer underflow\n") ;
            abort() ;
        }
    }

    size_t size() const {
        return dataEnd - dataStart ;
    }
} ;


struct Connection {
    int connectionFd = -1 ;
    bool wantToRead = false ;
    bool wantToWrite = false ;
    bool wantToClose = false ;

    Buffer* readBuffer ;
    Buffer* writeBuffer ;

    Connection(int fd)
        : connectionFd(fd), 
        wantToRead(true), 
        wantToWrite(false), 
        wantToClose(false),
        readBuffer(new Buffer(4096)),
        writeBuffer(new Buffer(4096)) {}

    ~Connection() {
        delete readBuffer;
        delete writeBuffer;
    }
} ;


static void die(const char* message) {
    fprintf(stderr, "[%d] %s\n", errno, message) ;
    abort() ;
}   

static void fdSetNonBlocking(int fd) {
    errno = 0 ;
    int flags = fcntl(fd, F_GETFL, 0) ;

    if (errno) {
        die("fcntl(F_GETFL)") ;
        return ;
    }

    flags |= O_NONBLOCK ;

    errno = 0 ;
    fcntl(fd, F_SETFL, flags) ;

    if (errno) {
        die("fcntl(F_SETFL)") ;
    }
}

bool readU32(
    const uint8_t*& data, 
    const uint8_t* end, 
    uint32_t& value
) {
    if (data + 4 > end) {
        return false ;
    }

    memcpy(&value, data, 4) ;
    value = ntohl(value) ;
    data += 4 ;

    return true ;
}

bool readString(
    const uint8_t*& data, 
    const uint8_t* end, 
    std::string& str, 
    uint32_t length
) {
    if (data + length > end) {
        return false ;
    }

    str.assign((const char*)data, length) ;
    data += length ;

    return true ;
}

static int32_t parseRequest(
    const uint8_t* data, 
    size_t size, 
    std::vector<std::string>& cmd
) {
    cmd.clear() ;
    const uint8_t* end = data + size ;
    uint32_t numStrings = 0 ;

    if (!readU32(data, end, numStrings)) {
        fprintf(stderr, "Invalid request length\n") ;
        return -1 ;
    }

    if (numStrings > kMaxArgs) {
        fprintf(stderr, "Too many arguments: %u\n", numStrings) ;
        return -1 ;
    }

    while (cmd.size() < numStrings) {
        uint32_t strLength = 0 ;

        if (!readU32(data, end, strLength)) {
            fprintf(stderr, "Invalid string length\n") ;
            return -1 ;
        }

        cmd.push_back(std::string()) ;

        if (!readString(data, end, cmd.back(), strLength)) {
            fprintf(stderr, "Invalid string data\n") ;
            return -1 ;
        }

    }

    if (data != end) {
        fprintf(stderr, "Extra data at the end of request\n") ;
        return -1 ;
    }

    return 0 ;
}


static void doRequest(
    const std::vector<std::string>& cmd, 
    Response& response
) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        auto it = g_data.find(cmd[1]) ;
        
        if (it == g_data.end()) {
            response.status = RES_NX ;
            return ;
        } 
        
        else {
            const std::string& value = it -> second ;
            response.data.assign(value.begin(), value.end()) ;
        }
    } 
    
    else if (cmd.size() == 3 && cmd[0] == "set") {
        g_data[cmd[1]] = cmd[2] ;
        response.status = RES_OK ;
    } 

    else if (cmd.size() == 2 && cmd[0] == "del") {
        auto it = g_data.find(cmd[1]) ;
        
        if (it == g_data.end()) {
            response.status = RES_NX ;
            return ;
        } 
        
        else {
            g_data.erase(it) ;
            response.status = RES_OK ;
        }
    }
    
    else if (cmd.size() == 1 && cmd[0] == "clear") {
        g_data.clear() ;
        response.status = RES_OK ;
    } 
    
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd[0].c_str()) ;
        response.status = RES_ERR ;
        return ;
    }
}

static void makeResponse(
    Buffer* writeBuffer, 
    const Response& response
) {
    uint32_t responseBodyLength = 4 + response.data.size() ;
    uint32_t netResponseLength = htonl(responseBodyLength) ;
    uint32_t netStatus = htonl(response.status) ;

    
    writeBuffer -> append((const uint8_t*)&netResponseLength, 4) ;
    writeBuffer -> append((const uint8_t*)&netStatus, 4) ;
    writeBuffer -> append(response.data.data(), response.data.size()) ;
}

//  ______________________________________
// | nstr | len | str1 | len | str2 | ... |
//  --------------------------------------
// where nstr is the number of strings, len is the length of each string


static bool tryOneRequest(Connection* conn) {

    if (conn -> readBuffer -> size() < 4) {
        return false ;
    }

    int32_t requestLength = 0 ;
    memcpy(&requestLength, conn -> readBuffer -> dataStart, 4) ; 
    requestLength = ntohl(requestLength) ;
    
    if (requestLength < 0 || (size_t)requestLength > kMaxMessage) {
        fprintf(stderr, "Invalid message length: %d\n", requestLength) ;
        conn -> wantToClose = true ;
        return false ;
    }

    if (conn -> readBuffer -> size() < (size_t)(4 + requestLength)) {
        return false ; 
    }

    const uint8_t* request = conn -> readBuffer -> dataStart + 4 ;

    printf("client(len:%d): %.*s\n", requestLength, requestLength < 100 ? requestLength : 100, request);

    std::vector<std::string> cmd ;

    if (parseRequest(request, requestLength, cmd) < 0) {
        fprintf(stderr, "Invalid request\n") ;
        conn -> wantToClose = true ;
        return false ;
    }

    Response response ;
    doRequest(cmd, response) ;
    makeResponse(conn -> writeBuffer, response) ;
    

    // conn -> writeBuffer -> append((const uint8_t*)& requestLength, 4) ;
    // conn -> writeBuffer -> append(request, requestLength) ;
    
    conn -> readBuffer -> consume(4 + requestLength) ;

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