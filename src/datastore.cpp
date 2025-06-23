#include "datastore.h"
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


static void doGet(
    std::vector<std::string>& cmd, 
    Response& out
) {
    Entry key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;
    
    if (!node) {
        out.status = RES_NX ;
        return ;
    }

    const std:: string &val = container_of(node, Entry, node) -> value ;
    assert(val.size() <= kMaxMessage) ;
    out.data.assign(val.begin(), val.end()) ;
}

static void doSet(
    std::vector<std::string>& cmd, 
    Response&
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
}

static void doDelete(
    std::vector<std::string>& cmd, 
    Response& 
) {
    Entry key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmDelete(&g_data.db, &key.node, &entryEq) ;
    
    if (node) {
        delete container_of(node, Entry, node) ;
    }
}


void doRequest(
    std::vector<std::string>& cmd, 
    Response& out
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

    else {
        fprintf(stderr, "Unknown command: %s\n", cmd[0].c_str()) ;
        out.status = RES_ERR ;
    }
}