#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>
#include "buffer.h"
#include "connection.h"

const size_t kMaxMessage = 32 << 20 ;
const size_t kMaxArgs = 200 * 1000;


bool tryOneRequest(Connection* conn) ;

#endif