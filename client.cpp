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

int32_t writeFull(int clientFd, const uint8_t* buffer, size_t size) {
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


int32_t readFull(int clientFd, uint8_t* buffer, size_t size) {
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


int32_t sendRequest(int fd, const uint8_t* data, size_t size) {
    if (size > kMaxMessage) {
        fprintf(stderr, "Message too long: %zu bytes\n", size) ;
        return -1 ;
    }

    std::vector<uint8_t> writeBuffer ;

    bufferAppend(writeBuffer, (const uint8_t*)&size, 4) ;
    bufferAppend(writeBuffer, data, size) ;

    int32_t err = writeFull(fd, writeBuffer.data(), writeBuffer.size()) ;

    if (err) {
        die("writeFull") ;
    }

    return 0 ;
}


int32_t readResponse(int fd) {
    std::vector<uint8_t> rbuf(4) ;
    errno = 0 ;

    int32_t err = readFull(fd, &rbuf[0], 4);

    if (err) {
        if (errno == 0) {
            fprintf(stderr, "%s\n", "EOF");
        }

        else {
            fprintf(stderr, "%s\n", "read() error");
        }

        return err ;
    }

    int responseLength = 0 ;
    memcpy(&responseLength, rbuf.data(), 4) ;

    if (responseLength > kMaxMessage) {
        fprintf(stderr, "Response too long: %d bytes\n", responseLength) ;
        return -1 ;
    }

    rbuf.resize(4 + responseLength) ;

    err = readFull(fd, &rbuf[4], responseLength) ;

    if (err) {
        fprintf(stderr, "read() error") ;
        return err ;
    }

    printf("len:%u data:%.*s\n", responseLength, responseLength < 100 ? responseLength : 100, &rbuf[4]) ;
    return 0 ;
}


int main() {
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

    std::vector<std::string> queryList = {
        "hello1", "hello2", "hello3",
        std::string(kMaxMessage, 'z'),
        "hello5",
    } ;

    for (const std::string &s : queryList) {
        int32_t err = sendRequest(fd, (uint8_t *)s.data(), s.size());

        if (err) {
            goto L_DONE;
        }
    }
    for (size_t i = 0; i < queryList.size(); ++i) {
        int32_t err = readResponse(fd);
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    close(fd);
    return 0;
}
