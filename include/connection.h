#ifndef CONNECTION_H
#define CONNECTION_H

#include "buffer.h"
#include "list.h"

struct Connection {
    int connectionFd = -1 ;
    bool wantToRead = false ;
    bool wantToWrite = false ;
    bool wantToClose = false ;

    Buffer* readBuffer ;
    Buffer* writeBuffer ;

    DList idleNode ;
    uint64_t lastActiveMS = 0 ;

    Connection(int fd) ;
    ~Connection() ; 
} ;

void connDestroy(Connection* conn) ;

#endif