#include "util.h"
#include "datastore.h"
#include "log.h"

#include <cstdio>
#include <cstdlib>  
#include <cerrno>   
#include <math.h>
#include <cstring>
#include <fcntl.h>


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



void outError(Buffer& out, uint32_t code, const std::string& msg) {
    return ;
}

void outNil(Buffer& out) {
    uint8_t tag = TAG_NIL ;
    out.append(&tag, 1) ;

    out.status = RES_NX ;
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

void outDouble(Buffer& out, double score) {

}


void outArray(Buffer& out, uint32_t n) {
    uint8_t tag = TAG_ARRAY ;
    out.append(&tag, 1) ;

    uint32_t netN = htonl(n) ;
    out.append((uint8_t*)&netN, sizeof(netN)) ;

    out.status = ResponseStatus::RES_OK ;
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