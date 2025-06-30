#ifndef HANDLER_H
#define HANDLER_H

#include "connection.h"


int32_t handleAccept(int serverFd) ;
void handleWrite(Connection * conn) ;
void handleRead(Connection * conn) ;

#endif