#ifndef DATASTORE_H
#define DATASTORE_H


#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) )) 


#include "protocol.h"
#include "hashtable.h"
#include <map>
#include <string>
#include <vector>
#include <cstddef>


enum ResponseStatus{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2
}; 

enum Tag {
    TAG_NIL = 0,
    TAG_ERROR = 1, 
    TAG_STRING = 2, 
    TAG_INT = 3, 
    TAG_DOUBLE = 4, 
    TAG_ARRAY = 5
} ;


struct {
    HMap db ;
} g_data ;


struct Entry {
    struct HNode node ;
    std::string key ;
    std::string value ;
} ;

void doRequest(std::vector<std::string>& cmd, Buffer& out) ;

#endif