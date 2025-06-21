#include "connection.h"


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
