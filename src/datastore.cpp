#include "datastore.h"
#include "hashtable.h"

#include <assert.h>

static bool entryEq(const HNode* lhs, const HNode* rhs) {
    Entry* leftEntry = container_of(lhs, Entry, node) ;
    Entry* rightEntry = container_of(rhs, Entry, node) ;

    return leftEntry -> key == rightEntry -> key ;
}

static uint64_t strHash(
    const uint8_t* data, 
    size_t len
) {
    uint64_t hash = 0x811C9DC5 ;

    for (size_t i = 0; i < len; i++) {
        hash = (hash + data[i]) * 0x01000193 ;
    }

    return hash ;
}


// static void outNil(Buffer& out) {
//     out.append((uint8_t*)"\x02", Tag::TAG_NIL) ; 
// }

static void outNil(Buffer& out) {
    uint8_t tag = TAG_NIL ;
    out.append(&tag, 1) ;

    out.status = RES_NX ;
}


static void outString(Buffer& out, const char* s, size_t size) {
    if (size == 0) {
        out.append((uint8_t*)"", 0) ; 
        return ;
    }    

    uint8_t tag = TAG_STRING ;
    out.append(&tag, 1) ;

    uint32_t len = htonl((uint32_t)size) ;
    out.append((uint8_t*)&len, 4) ;

    out.append((uint8_t*)s, size) ;

    out.status = RES_OK ;
}


static void outInt(Buffer& out, uint8_t& node) {
    uint8_t tag = TAG_INT ;
    out.append(&tag, 1) ;

    int64_t beValue = htobe64(value) ; 
    out.append((uint8_t*)&beValue, sizeof(beValue)) ;

    out.status = ResponseStatus::RES_OK ;
}


static void outArray(Buffer& out, uint32_t n) {
    uint8_t tag = TAG_ARRAY ;
    out.append(&tag, 1) ;

    uint32_t netN = htonl(n) ;
    out.append((uint8_t*)&netN, sizeof(netN)) ;

    out.status = ResponseStatus::RES_OK ;
}


static void doGet(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    Entry key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;
    
    if (!node) {
        out.status = ResponseStatus::RES_NX ;
        return ;
    }

    const std:: string &val = container_of(node, Entry, node) -> value ;
    
    return outString(out, val.data(), val.size()) ;
}

static void doSet(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    Entry key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;
    
    if (node) {
        container_of(node, Entry, node) -> value.swap(cmd[2]) ;
    }

    else {
        Entry *entry = new Entry() ;
        entry -> key.swap(key.key) ;
        entry -> node.hash = key.node.hash ;
        entry -> value.swap(cmd[2]) ;

        hmInsert(&g_data.db, &entry -> node) ;
    }

    return outNil(out) ;
}

static void doDelete(
    std::vector<std::string>& cmd, 
    Buffer& out
) {
    Entry key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmDelete(&g_data.db, &key.node, &entryEq) ;
    
    if (node) {
        delete container_of(node, Entry, node) ;
    }

    uint8_t flag = node ? 1 : 0;

    return (outInt(out, flag)) ;
}

static bool cbKeys(HNode* node, void* arg) {
    Buffer& out = *(Buffer*)arg ;
    const std::string& key = container_of(node, Entry, node) -> key ;
    outString(out, key.data(), key.size()) ;
    
    return true ;
}


static void doKeys(
    Buffer& out
) {
    outArray(out, (uint32_t)hmSize(&g_data.db)) ;
    hmForEach(&g_data.db, &cbKeys, (void*)& out) ;
}


void doRequest(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    if (cmd.size() == 2 && cmd[0] == "get") {
        return doGet(cmd, out) ;
    } 
    
    else if (cmd.size() == 3 && cmd[0] == "set") {
        return doSet(cmd, out) ;
    } 

    else if (cmd.size() == 2 && cmd[0] == "del") {
        return doDelete(cmd, out) ;
    }

    else if (cmd.size() == 1 && cmd[0] == "keys") {
        return doKeys(out) ;
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", cmd[0].c_str()) ;
        out.status = ResponseStatus::RES_ERR ;
    }
}