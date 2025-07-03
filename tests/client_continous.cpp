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
#include <string>
#include <chrono>
#include <iostream>  
#include <cstdint>


#ifdef _WIN32
    #include <winsock2.h>
    #define be64toh(x) _byteswap_uint64(x)
#elif defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define be64toh(x) OSSwapBigToHostInt64(x)
#else
    #include <endian.h>
#endif
  

enum Tag {
    TAG_NIL = 0,
    TAG_ERROR = 1,
    TAG_STRING = 2,
    TAG_INT = 3,
    TAG_DOUBLE = 4,
    TAG_ARRAY = 5
};


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
    uint32_t netLen;
    if (read(fd, &netLen, 4) != 4) {
        std::cerr << "Failed to read response length\n";
        return 1;
    }

    uint32_t len = ntohl(netLen);
    if (len == 0 || len > 1024 * 1024) {
        std::cerr << "Invalid response length: " << len << " bytes\n";
        return 1;
    }

    

    std::vector<uint8_t> buffer(len);
    if (read(fd, buffer.data(), len) != (ssize_t)len) {
        std::cerr << "Failed to read full response payload\n";
        return 1;
    }

    

    size_t offset = 0;
    uint8_t tag = buffer[offset++];

    switch (tag) {
        case TAG_NIL:
            // std::cout << "status: NX\n";
            // std::cout << "data: (nil)\n";
            break;

        case TAG_STRING: {
            if (offset + 4 > len) {
                return 1;
            }

            uint32_t strLen;
            memcpy(&strLen, buffer.data() + offset, 4);
            strLen = ntohl(strLen);
            offset += 4;

            if (offset + strLen > len) {
                return 1;
            }

            std::string value((char*)(buffer.data() + offset), strLen);
            // std::cout << "status: OK\n";
            // std::cout << "data: " << value << "\n";
            break;
        }

        case TAG_INT: {
            if (offset + 8 > len) return 1;
            int64_t val;
            memcpy(&val, buffer.data() + offset, 8);
            val = be64toh(val);
            // std::cout << "status: OK\n";
            // std::cout << "data: " << val << "\n";
            break;
        }

        case TAG_ARRAY: {
            if (offset + 4 > len) return 1;

            uint32_t arraySize;
            memcpy(&arraySize, buffer.data() + offset, 4);
            arraySize = ntohl(arraySize);
            offset += 4;

            // std::cout << "status: OK\n";
            // std::cout << "data: [";

            for (uint32_t i = 0; i < arraySize; ++i) {
                if (offset >= len) return 1;

                uint8_t itemTag = buffer[offset++];
                switch (itemTag) {
                    case TAG_STRING: {
                        if (offset + 4 > len) return 1;
                        uint32_t itemLen;
                        memcpy(&itemLen, buffer.data() + offset, 4);
                        itemLen = ntohl(itemLen);
                        offset += 4;

                        if (offset + itemLen > len) return 1;
                        std::string itemValue((char*)(buffer.data() + offset), itemLen);
                        offset += itemLen;

                        // std::cout << "\"" << itemValue << "\"";
                        break;
                    }
                    case TAG_INT: {
                        if (offset + 8 > len) return 1;
                        int64_t itemVal;
                        memcpy(&itemVal, buffer.data() + offset, 8);
                        itemVal = be64toh(itemVal);
                        offset += 8;

                        // std::cout << itemVal;
                        break;
                    }
                    default:
                        // std::cerr << "Unknown array item tag: " << (int)itemTag << "\n";
                        return 1;
                }

                if (i < arraySize - 1) {
                    // std::cout << ", ";
                }
            }

            // std::cout << "]\n";
            break;
        }

        case TAG_ERROR:
            // std::cerr << "status: ERR\n";
            // std::cerr << "data: (error message)\n"; 
            break;

        

        default:
            // std::cerr << "Unknown tag: " << (int)tag << "\n";
            return 1;
    }

    return 0;
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0) ;

    if (fd < 0) {
        die("socket") ;
    }

    // else {
    //     printf("Socket created successfully with file descriptor: %d\n", fd) ;
    // }

    struct sockaddr_in serverAddress = {} ;
    serverAddress.sin_family = AF_INET ;
    serverAddress.sin_port = htons(1234) ;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); ;

    int connectionResult = connect(fd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) ;

    if (connectionResult < 0) {
        die("connect") ;
    }

    // Replace your existing commands vector-building block with this:

    std::vector<std::vector<std::string>> setCommands;
    std::vector<std::vector<std::string>> getCommands;
    std::vector<std::vector<std::string>> delCommands;

    // Prepare 1000 SETs
    for (int i = 0; i < 1000; ++i) {
        setCommands.push_back({"set", "key" + std::to_string(i), "value" + std::to_string(i)});
    }

    // Prepare 1000 GETs
    for (int i = 0; i < 1000; ++i) {
        getCommands.push_back({"get", "key" + std::to_string(i)});
    }

    // Prepare 1000 DELs
    for (int i = 0; i < 1000; ++i) {
        delCommands.push_back({"del", "key" + std::to_string(i)});
    }

    // Benchmark SETs
    auto startSet = std::chrono::steady_clock::now();
    for (const auto& cmd : setCommands) {
        if (sendRequest(fd, cmd)) die("sendRequest SET");
    }
    for (size_t i = 0; i < setCommands.size(); ++i) {
        if (readResponse(fd)) die("readResponse SET");
    }
    auto endSet = std::chrono::steady_clock::now();
    double elapsedSetMs = std::chrono::duration_cast<std::chrono::microseconds>(endSet - startSet).count() / 1000.0;

    // Benchmark GETs
    auto startGet = std::chrono::steady_clock::now();
    for (const auto& cmd : getCommands) {
        if (sendRequest(fd, cmd)) die("sendRequest GET");
    }
    for (size_t i = 0; i < getCommands.size(); ++i) {
        if (readResponse(fd)) die("readResponse GET");
    }
    auto endGet = std::chrono::steady_clock::now();
    double elapsedGetMs = std::chrono::duration_cast<std::chrono::microseconds>(endGet - startGet).count() / 1000.0;

    // Benchmark DELs
    auto startDel = std::chrono::steady_clock::now();
    for (const auto& cmd : delCommands) {
        if (sendRequest(fd, cmd)) die("sendRequest DEL");
    }
    for (size_t i = 0; i < delCommands.size(); ++i) {
        readResponse(fd) ;
    }
    auto endDel = std::chrono::steady_clock::now();
    double elapsedDelMs = std::chrono::duration_cast<std::chrono::microseconds>(endDel - startDel).count() / 1000.0;

    printf("\n--- Benchmarking custom key-value store ---\n");
    printf("Avg set time: %.3f ms\n", elapsedSetMs/setCommands.size());
    printf("Avg get time: %.3f ms\n", elapsedGetMs/getCommands.size());
    printf("Avg detete time: %.3f ms\n", elapsedDelMs/delCommands.size());



}
