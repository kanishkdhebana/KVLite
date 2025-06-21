#include "buffer.h"
#include <cstring>
#include <cstdio>


Buffer::Buffer(size_t size) {
    bufferBegin = (uint8_t*)malloc(size) ;

    if (!bufferBegin) {
        perror("malloc") ;
        exit(EXIT_FAILURE) ;
    }

    bufferEnd = bufferBegin + size ;
    dataStart = bufferBegin ;
    dataEnd = bufferBegin ;
}
    
Buffer::~Buffer() {
    free(bufferBegin) ;
}

void Buffer::append(
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

void Buffer::consume(  
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

size_t Buffer::size() const {
    return dataEnd - dataStart ;
}
