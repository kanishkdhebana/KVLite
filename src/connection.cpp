#include "connection.h"
#include "list.h"
#include "util.h"
#include "datastore.h"

#include <unistd.h> 


Connection::Connection(int fd)
    : connectionFd(fd),
      wantToRead(true),
      wantToWrite(false),
      wantToClose(false),
      readBuffer(new Buffer(4096)),
      writeBuffer(new Buffer(4096)) {}

Connection::~Connection() {
    delete readBuffer ;
    delete writeBuffer ;
}


void connDestroy(Connection* conn) {
    (void)close(conn -> connectionFd) ;
    g_data.fd2conn[conn -> connectionFd] = NULL ;
    dlistDetach(&conn -> idleNode) ;
    
    delete conn ;
}