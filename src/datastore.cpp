#include "datastore.h"  
#include "util.h"
#include "helper_time.h"
#include "zset.h"

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

void entrySetTTL(Entry* entry, int64_t ttlMS) {
    if (ttlMS < 0 && entry -> heapIdx != (size_t)-1) {
        heapDelete(g_data.heap, entry -> heapIdx) ;
        entry -> heapIdx = -1 ;
    }

    else if (ttlMS >= 0) {
        uint64_t expireAt = getMonoticMS() + (uint64_t)ttlMS ;
        HeapItem item = {expireAt, &entry -> heapIdx} ;
        heapInsert(g_data.heap, entry -> heapIdx, item) ;
    }

    
}

static void doExpire(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    int64_t ttlMS = 0 ;
    
    if (!str2Int(cmd[2], ttlMS)) {
        return outError(out, ERR_BAD_ARG, "expect int64") ;
    }

    LookupKey key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;
    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;

    if (node) {
        Entry* entry = container_of(node, Entry, node) ;
        entrySetTTL(entry, ttlMS) ;
    }

    return outInt(out, node? 1 : 0) ;
}


static void doTTL(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    LookupKey key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;
    HNode* node = hmLookup(&g_data.db, &key.node, &entryEq) ;

    if (!node) {
        return outInt(out, -2) ; // no key
    }

    Entry* entry = container_of(node, Entry, node) ;
    if (entry -> heapIdx == (size_t)-1) {
        return outInt(out, -1) ; // no ttl
    }

    uint64_t expireAt = g_data.heap[entry -> heapIdx].val ;
    uint64_t nowMS = getMonoticMS() ;


    return outInt(out, expireAt > nowMS ? (expireAt - nowMS) : 0) ;
}


static void doZAdd(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    int64_t score = 0 ;
    
    if (!str2Int(cmd[2], score)) {
        return outError(out, ERR_BAD_ARG, "expect int64") ;
    }


    LookupKey key ;
    key.key.swap(cmd[1]) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;
    HNode* hnode = hmLookup(&g_data.db, &key.node, &entryEq) ;

    Entry* entry = NULL ;

    if (!hnode) {
        entry = entryNew(T_ZSET) ;
        entry -> key.swap(key.key) ;
        entry -> node.hash = key.node.hash ;

        entry -> zset = ZSet();

        hmInsert(&g_data.db, &entry -> node) ;
    }

    else {
        entry = container_of(hnode, Entry, node) ;

        if (entry -> type != T_ZSET) {
            return outError(out, ERR_BAD_TYP, "expect zset") ;
        }
    }

    const std::string& name = cmd[3] ;

    printf("Inserting into ZSet: %s, score: %lld\n", name.c_str(), score) ;

    bool added = zsetInsert(&entry -> zset, name.data(), name.size(), score) ;

    return outInt(out, (int64_t)added) ;
}

static const ZSet kEmptyZSet;

static ZSet* expectZset(std::string& s) {
    LookupKey key ;
    key.key.swap(s) ;
    key.node.hash = strHash((uint8_t*)key.key.data(), key.key.size()) ;
    HNode* hnode = hmLookup(&g_data.db, &key.node, &entryEq) ;

    if (!hnode) {
        return (ZSet*)&kEmptyZSet ;
    }

    Entry* entry = container_of(hnode, Entry, node) ;
    
    return entry -> type == T_ZSET ? &entry -> zset : NULL ;
}


static void doZRemove(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    ZSet* zset = expectZset(cmd[1]) ;

    if (!zset) {
        return outError(out, ERR_BAD_TYP, "expect zset") ;
    }

    const std::string& name = cmd[2] ;
    ZNode* znode = zsetLookup(zset, name.data(), name.size()) ;

    if (znode) {
        zsetDelete(zset, znode) ;
    }

    return outInt(out, znode ? 1 : 0) ;
}

static void doZScore(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    ZSet* zset = expectZset(cmd[1]) ;
    

    if (!zset) {
        return outError(out, ERR_BAD_TYP, "expect zset") ;
    }

    const std::string& name = cmd[2] ;
    printf("Looking up score for: %s\n", cmd[2].c_str());

    ZNode* znode = zsetLookup(zset, name.data(), name.size()) ;

    if (!znode) {
        printf("znode is NULL") ;
    }

    else {
        printf("znode: %f\n", znode -> score) ;
    }

    return znode ? outDouble(out, znode -> score) : outNil(out) ;
}


static void doZQuery(
    std::vector<std::string>& cmd,
    Buffer& out
) {
    double score = 0 ;

    if (!str2Double(cmd[2], score)) {
        return outError(out, ERR_BAD_ARG, "expect floating-point number") ;
    }

    const std::string& name = cmd[3] ;
    int64_t offset = 0 ;
    int64_t limit = 0 ;

    if (!str2Int(cmd[4], offset) || !str2Int(cmd[5], limit)) {
        return outError(out, ERR_BAD_ARG, "expect int") ;
    }

    ZSet* zset = expectZset(cmd[1]) ;

    if (!zset) {
        return outError(out, ERR_BAD_TYP, "expect zset") ;
    }

    if (limit <= 0) {
        return outArray(out, 0) ;
    }

    ZNode* znode = zsetSeekage(zset, score, name.data(), name.size()) ;
    znode = znodeOffset(znode, offset) ;

    size_t ctx = outBeginArray(out) ;
    int64_t n = 0 ;

    while (znode && n < limit) {
        outString(out, znode -> name, znode -> len) ;
        outDouble(out, znode -> score) ;
        znode = znodeOffset(znode, +1) ;
        n += 2 ;
    }

    outEndArray(out, ctx, (uint32_t)n) ; 
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

    else if (cmd.size() == 3 && cmd[0] == "expire") {
        return doExpire(cmd, out) ;
    }

    else if (cmd.size() == 2 && cmd[0] == "pttl") {
        return doTTL(cmd, out) ;
    }

    else if (cmd.size() == 4 && cmd[0] == "zadd") {
        return doZAdd(cmd, out) ;
    }

    else if (cmd.size() == 3 && cmd[0] == "zrem") {
        return doZRemove(cmd, out) ;
    }

    else if (cmd.size() == 3 && cmd[0] == "zscore") {
        return doZScore(cmd, out) ;
    }

    else if (cmd.size() == 6 && cmd[0] == "zquery") {
        return doZQuery(cmd, out) ;
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", cmd[0].c_str()) ;
        out.status = ResponseStatus::RES_ERR ;
    }
}