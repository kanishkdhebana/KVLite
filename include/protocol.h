#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>
#include "buffer.h"
#include "connection.h"


struct Response {
    uint32_t status = 0 ;
    std::vector<uint8_t> data ;
} ;

bool readU32(const uint8_t*& data, const uint8_t* end, uint32_t& value) ;
bool readString(const uint8_t*& data, const uint8_t* end, std::string& str, uint32_t length) ;
int32_t parseRequest(const uint8_t* data, size_t size, std::vector<std::string>& cmd) ; 
void makeResponse(Buffer* writeBuffer, const Response& response) ;
bool tryOneRequest(Connection* conn) ;

#endif