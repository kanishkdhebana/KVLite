#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <cstdlib>

struct Buffer {
    uint8_t* bufferBegin ;
    uint8_t* bufferEnd ;
    uint8_t* dataStart ;
    uint8_t* dataEnd ;
    uint32_t status = 0 ;

    Buffer(size_t size) ;
    ~Buffer() ;
    void append(const uint8_t* data, size_t size) ;
    void consume(size_t size) ;
    size_t size() const ;
} ;

#endif 