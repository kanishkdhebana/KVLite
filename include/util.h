#ifndef UTILS_H
#define UTILS_H

#include "buffer.h"

#include <stdint.h>
#include <stddef.h>
#include <string>


#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))


uint64_t strHash(const uint8_t* data, size_t len) ;

void die(const char* message) ;
void fdSetNonBlocking(int fd) ;

void outError(Buffer& out, uint32_t code, const std::string& msg) ;
void outNil(Buffer& out) ;
void outString(Buffer& out, const char* s, size_t size) ;
void outInt(Buffer& out, uint64_t val) ;
void outDouble(Buffer& out, double score) ;
void outArray(Buffer& out, uint32_t n) ;
size_t outBeginArray(Buffer& out) ;
void outEndArray(Buffer& out, size_t ctx, uint32_t n) ;

bool str2Double(const std::string& s, double& out) ;
bool str2Int(const std::string& s, int64_t& out) ;

#endif