#ifndef DATASTORE_H
#define DATASTORE_H

#include "protocol.h"
#include "hashtable.h"
#include "zset.h"
#include "heap.h"
#include "thread_pool.h"

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

enum EntryType {
    T_INIT = 0,
    T_STR = 1,
    T_ZSET = 2
} ;

enum Error{
    ERR_UNKNOWN = 1,  
    ERR_TOO_BIG = 2,
    ERR_BAD_TYP = 3,  
    ERR_BAD_ARG = 4
};


struct GData {
    HMap db ;

    std::vector<Connection*> fd2conn ;
    DList idleList ;
    std::vector<HeapItem> heap ;
    ThreadPool thread_pool ;
} ;

extern GData g_data; 

struct Entry {
    struct HNode node ;
    std::string key ;
    
    uint32_t type = 0 ;

    std::string str ;
    ZSet zset ;

    size_t heapIdx = -1 ;
} ;


struct LookupKey {
    struct HNode node ; 
    std::string key ;
};

void doRequest(std::vector<std::string>& cmd, Buffer& out) ;
bool entryEq(const HNode *node, const HNode *key) ;

void entrySetTTL(Entry* entry, int64_t ttlMS) ;

#endif