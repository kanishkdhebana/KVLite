#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

const size_t kMaxMessage = 4096 ;

static void die(const char* message) {
    fprintf(stderr, "[%d] %s\n", errno, message) ;
    abort() ;
}

static int32_t writeAll(int clientFd, const uint8_t* buffer, size_t size) {
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


static int32_t readFull(int clientFd, uint8_t* buffer, size_t size) {
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


static void bufferAppend(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}


static int32_t sendRequest(int fd, const std::vector<std::string> &cmd) {
    uint32_t numArgs = cmd.size() ;
    uint32_t payloadLength = 4 ;

    for (const auto &arg : cmd) {
        payloadLength += 4 + arg.length() ; 
    }

    if (payloadLength > kMaxMessage) {
        fprintf(stderr, "Message too long: %zu bytes\n", payloadLength) ;
        return -1 ;
    }


    std::vector<uint8_t> writeBuffer ;
    writeBuffer.reserve(4 + payloadLength) ;

    uint32_t netPayloadLength = htonl(payloadLength) ;

    bufferAppend(writeBuffer, (const uint8_t*)&netPayloadLength, 4) ;

    uint32_t netNumArgs = htonl(numArgs) ;
    bufferAppend(writeBuffer, (const uint8_t*)&netNumArgs, 4) ;

    for (const auto &arg : cmd) {
        uint32_t argLength = arg.length() ;

        uint32_t netArgLength = htonl(argLength);
        bufferAppend(writeBuffer, (const uint8_t*)&netArgLength, 4) ;
        bufferAppend(writeBuffer, (const uint8_t*)arg.data(), argLength) ;
    }

    return writeAll(fd, writeBuffer.data(), writeBuffer.size()) ;
}


static int32_t readResponse(int fd) {
    errno = 0 ;
    uint8_t lenBuffer[4] ;

    int32_t err = readFull(fd, lenBuffer, 4) ;

    if (err < 0) {
        if (errno == 0) {
            fprintf(stderr, "%s\n", "EOF");
        } 
        
        else {
            fprintf(stderr, "%s\n", "read() error");
        }

        return -1 ;
    }

    uint32_t responseLength = 0 ;
    memcpy(&responseLength, lenBuffer, 4) ;
    responseLength = ntohl(responseLength) ;

    if (responseLength > kMaxMessage) {
        fprintf(stderr, "Response too long: %u bytes\n", responseLength) ;
        return -1 ;
    }

    std::vector<uint8_t> rbuf(responseLength) ;

    err = readFull(fd, rbuf.data(), responseLength) ;

    if (err < 0) {
        if (errno == 0) {
            fprintf(stderr, "%s\n", "EOF");
        } 
        
        else {
            fprintf(stderr, "%s\n", "read() error");
        }

        return -1 ;
    }

    if (responseLength < 4) {
        fprintf(stderr, "Invalid response length: %u bytes\n", responseLength) ;
        return -1 ;
    }

    uint32_t status = 0 ;
    memcpy(&status, rbuf.data(), 4) ;
    status = ntohl(status) ;

    const char *dataPtr = (const char*)rbuf.data() + 4 ;
    size_t dataLength = responseLength - 4 ;        

    printf("  status: %u\n", status);
    printf("  data: %.*s\n", (int)dataLength, dataPtr);

    return 0;
}


int main(int argc, char **argv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0) ;

    if (fd < 0) {
        die("socket") ;
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
        die("connect") ;
    }

    std::vector<std::string> cmd ;

    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]) ;
    } ;

    int32_t err = sendRequest(fd, cmd);

    if (err) {
        fprintf(stderr, "sendRequest failed\n") ;
        goto L_DONE ;
    }

    err = readResponse(fd);
    if (err) {
        fprintf(stderr, "readResponse failed\n") ;
        goto L_DONE ;
    }   

L_DONE:
    close(fd);
    return 0;
}
