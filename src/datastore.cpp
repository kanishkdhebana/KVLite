#include "datastore.h"  
#include "util.h"

#include <assert.h>

GData g_data ;

bool entryEq(const HNode *node, const HNode *key) {
    struct Entry* ent = container_of(node, Entry, node) ;
    struct LookupKey* keydata = container_of(key, LookupKey, node) ;
    return ent -> key == keydata -> key ;
}


// static void outNil(Buffer& out) {
//     out.append((uint8_t*)"\x02", Tag::TAG_NIL) ; 
// }




static void doGet(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    LookupKey key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;
    
    if (!node) {
        return outNil(out) ;
    }

    Entry* entry = container_of(node, Entry, node) ;

    if (entry -> type != T_STR) {
        return outError(out, ERR_BAD_TYP, "not a string value") ;
    } 
    
    return outString(out, entry -> str.data(), entry -> str.size()) ;
}


static Entry* entryNew(uint32_t type) {
    Entry *ent = new Entry() ;
    ent -> type = type ;
    return ent ;
}


static void doSet(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    LookupKey key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;

    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;
    
    if (node) {
        Entry* entry = container_of(node, Entry, node) ;

        if (entry -> type != T_STR) {
            return outError(out, ERR_BAD_TYP, "a non-string value exists.") ;
        }

        entry -> str.swap(cmd[2]) ;
    }

    else {
        Entry *entry = entryNew(T_STR) ;
        entry -> key.swap(key.key) ;
        entry -> node.hash = key.node.hash ;
        entry -> str.swap(cmd[2]) ;

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