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

struct Response ;

// enum {
//     RES_OK = 0,
//     RES_ERR = 1,
//     RES_NX = 2
// };

constexpr int RES_OK = 0;
constexpr int RES_NX = 1;
constexpr int RES_ERR = 2;

struct {
    HMap db ;
} g_data ;


struct Entry {
    struct HNode node ;
    std::string key ;
    std::string value ;
} ;

void doRequest(std::vector<std::string>& cmd, Response& out) ;

#endif