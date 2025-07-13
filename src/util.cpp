#include "util.h"
#include "datastore.h"
#include "log.h"

#include <cstdio>
#include <cstdlib>  
#include <cerrno>   
#include <math.h>
#include <cstring>
#include <fcntl.h>
#include <cassert>
#include <iostream>


uint64_t strHash(
    const uint8_t* data, 
    size_t len
) {
    uint64_t hash = 0x811C9DC5 ;

    for (size_t i = 0; i < len; i++) {
        hash = (hash + data[i]) * 0x01000193 ;
    }

    return hash ;
}
  

void die(const char* message) {
    logMessage("%s: %s", message, strerror(errno));
    exit(EXIT_FAILURE) ;
}


void fdSetNonBlocking(int fd) {
    // errno = 0 ;
    int flags = fcntl(fd, F_GETFL, 0) ;

    // if (errno) {
    //     die("fcntl(F_GETFL)") ;
    //     return ;
    // }

    if (flags == -1) {
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



void outError(Buffer& out, uint32_t code, const std::string& msg) {
    uint8_t tag = TAG_ERROR ;
    out.append(&tag, 1) ;
    uint32_t netCode = htonl(code) ;
    out.append((uint8_t*)&netCode, sizeof(netCode)) ;
    uint32_t msgSize = htonl((uint32_t)msg.size()) ;
    out.append((uint8_t*)&msgSize, sizeof(msgSize)) ;
    out.append((uint8_t*)msg.data(), msg.size()) ;

    out.status = RES_ERR ;
}

void outNil(Buffer& out) {

    printf("\noutNIl\n") ;
    uint8_t tag = TAG_NIL ;
    out.append(&tag, 1) ;

    out.status = RES_OK ;
}


void outString(Buffer& out, const char* s, size_t size) {
    if (size == 0) {
        out.append((uint8_t*)"", 0) ; 
        return ;
    }    

    uint8_t tag = TAG_STRING ;
    out.append(&tag, 1) ;

    uint32_t len = htonl((uint32_t)size) ;
    out.append((uint8_t*)&len, 4) ;

    out.append((uint8_t*)s, size) ;

    out.status = RES_OK ;
}


void outInt(Buffer& out, uint64_t val) {
    uint8_t tag = TAG_INT ;
    out.append(&tag, 1) ;
    int32_t netValue = htonl((int32_t)val) ;
    out.append((uint8_t*)&netValue, sizeof(netValue)) ;
    out.status = ResponseStatus::RES_OK ;
}

void outDouble(Buffer& out, double val) {
    std::cout << "outDouble" << std::endl ;


    uint8_t tag = TAG_DOUBLE ;
    out.append(&tag, 1) ;
    uint64_t netValue = htonll((uint64_t)val) ;
    out.append((uint8_t*)&netValue, sizeof(netValue)) ;

    out.status = ResponseStatus::RES_OK ;
}


void outArray(Buffer& out, uint32_t n) {
    uint8_t tag = TAG_ARRAY ;
    out.append(&tag, 1) ;

    uint32_t netN = htonl(n) ;
    out.append((uint8_t*)&netN, sizeof(netN)) ;

    out.status = ResponseStatus::RES_OK ;
}


size_t outBeginArray(Buffer& out) {
    uint8_t tag = TAG_ARRAY ;
    out.append(&tag, 1) ;

    size_t ctx = out.size() ;
    uint32_t n = 0 ;
    out.append((uint8_t*)&n, sizeof(n)) ; 

    return ctx - 4 ;
}


void outEndArray(Buffer& out, size_t ctx, uint32_t n) {
    assert(out.dataStart[ctx - 1] == TAG_ARRAY) ;
    uint32_t netN = htonl(n) ;
    out.append((uint8_t*)&netN, sizeof(netN)) ;
}


bool str2Double(const std::string& s, double& out) {
    char* endpoint = NULL ;
    out = strtod(s.c_str(), &endpoint) ;

    return endpoint == s.c_str() + s.size() && !isnan(out) ;
}

bool str2Int(const std::string& s, int64_t& out) {
    char* endpoint = NULL ;
    out = strtoll(s.c_str(), &endpoint, 10) ;

    return endpoint == s.c_str() + s.size() && !isnan(out) && out >= 0 ;
}
