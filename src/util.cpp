#include "util.h"
#include <cstdio>
#include <cstdlib>  
#include <cerrno>   
#include <fcntl.h>
  
 


void die(const char* message) {
    perror(message) ;
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

