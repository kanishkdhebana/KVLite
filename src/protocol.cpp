#include "protocol.h"
#include "connection.h"
#include "buffer.h"
#include "datastore.h"
#include <cstring>


static bool readU32(
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


static bool readString(
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

    // printf("Parsed command: ");
    // for (size_t i = 0; i < cmd.size(); ++i) {
    //     printf("%s%s", cmd[i].c_str(), i + 1 == cmd.size() ? "\n" : " ");
    // }

    return 0 ;
}


// void makeResponse(
//     Buffer* writeBuffer, 
//     const Response& response
// ) {
//     uint32_t responseBodyLength = 4 + response.data.size() ;
//     uint32_t netResponseLength = htonl(responseBodyLength) ;
//     uint32_t netStatus = htonl(response.status) ;

    
//     writeBuffer -> append((const uint8_t*)&netResponseLength, 4) ;
//     writeBuffer -> append((const uint8_t*)&netStatus, 4) ;
//     writeBuffer -> append(response.data.data(), response.data.size()) ;
// }

static void responseBegin(Buffer& out, size_t* header) {
    *header = out.size() ;
    out.append((const uint8_t*)"\x00\x00\x00\x00", 4) ; // placeholder for length
}

static size_t responseSize(Buffer& out, size_t header) {
    return out.size() - header - 4 ;
}

static void responseEnd(Buffer& out, size_t* header) {
    size_t messageSize = responseSize(out, *header) ;

    if (messageSize > kMaxMessage) {
        fprintf(stderr, "Response too large: %zu bytes\n", messageSize) ;
        out.status = ResponseStatus::RES_ERR ;
        out.consume(messageSize + 4) ; 
        return ;      
    }


    uint32_t netMessageSize = htonl((uint32_t)messageSize) ;
    memcpy(out.dataStart + *header, &netMessageSize, 4) ; 
}


//  ______________________________________
// | nstr | len | str1 | len | str2 | ... |
//  --------------------------------------
// where nstr is the number of strings, len is the length of each string


bool tryOneRequest(Connection* conn) {
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

    std::vector<std::string> cmd ;

    if (parseRequest(request, requestLength, cmd) < 0) {
        fprintf(stderr, "Invalid request\n") ;
        conn -> wantToClose = true ;
        return false ;
    }

    // printf("Parsed command: ");
    // for (size_t i = 0; i < cmd.size(); ++i) {
    //     printf("%s%s", cmd[i].c_str(), i + 1 == cmd.size() ? "\n" : " ");
    // }


    size_t headerPos = 0 ;

    responseBegin(*conn -> writeBuffer, &headerPos) ;
    doRequest(cmd, *conn -> writeBuffer) ;
    responseEnd(*conn -> writeBuffer, &headerPos) ;

   

    // conn -> writeBuffer -> append((const uint8_t*)& requestLength, 4) ;
    // conn -> writeBuffer -> append(request, requestLength) ;
    
    conn -> readBuffer -> consume(4 + requestLength) ;

    return true ;
}