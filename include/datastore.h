#ifndef DATASTORE_H
#define DATASTORE_H

#include "protocol.h"
#include <map>
#include <string>
#include <vector>

struct Response ;
extern std::map<std::string, std::string> g_data ;

// enum {
//     RES_OK = 0,
//     RES_ERR = 1,
//     RES_NX = 2
// };

constexpr int RES_OK = 0;
constexpr int RES_NX = 1;
constexpr int RES_ERR = 2;

void doRequest(const std::vector<std::string>& cmd, Response& response) ;


#endif