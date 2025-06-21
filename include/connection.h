#ifndef CONNECTION_H
#define CONNECTION_H

#include "buffer.h"

struct Connection {
    int connectionFd = -1 ;
    bool wantToRead = false ;
    bool wantToWrite = false ;
    bool wantToClose = false ;

    Buffer* readBuffer ;
    Buffer* writeBuffer ;

    Connection(int fd) ;
    ~Connection() ; 
} ;

#endif