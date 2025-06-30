#include "zset.h"
#include "util.h"
#include "buffer.h"
#include "datastore.h"
#include "avl.h"

#include <assert.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <vector>
#include <math.h>

static const ZSet k_empty_zset;

static bool str2Double(const std::string& s, double& out) {
    char* endpoint = NULL ;
    out = strtod(s.c_str(), &endpoint) ;

    return endpoint == s.c_str() + s.size() && !isnan(out) ;
}

static bool str2Int(const std::string& s, int64_t& out) {
    char* endpoint = NULL ;
    out = strtol(s.c_str(), &endpoint, 10) ;

    return endpoint == s.c_str() + s.size() && !isnan(out) && out >= 0 ;
}

static size_t outBeginArray(Buffer& out) {
    uint8_t tag = TAG_ARRAY ;
    out.append(&tag, 1) ;

    size_t ctx = out.size() ;
    uint32_t n = 0 ;
    out.append((uint8_t*)&n, sizeof(n)) ; 

    return ctx - 4 ;
}


static void outEndArray(Buffer& out, size_t ctx, uint32_t n) {
    // assert(out[ctx - 1] == TAG_ARR);
    // memcpy(&out[ctx], &n, 4);
    out.append((uint8_t*)&n, sizeof(n)) ; // append size of array
}

static ZNode* znodeNew(
    const char* name, 
    size_t len, 
    double score
) {
    ZNode* node = (ZNode*)malloc(sizeof(ZNode) + len) ;
    avlInit(&node -> tree) ;
    
    node -> hmap.next = NULL ;
    node -> hmap.hash = strHash((uint8_t*)name, len) ;
    node -> score = score ;
    node -> len = len ;
    memcpy(&node -> name[0], name, len) ;
    
    return node ;
}

static void znodeDelete(ZNode* node) {
    free(node) ;
}


static bool hcmp(
    const HNode* node, 
    const HNode* key
) {
    ZNode* znode = container_of(node, ZNode, hmap) ;
    HKey* hkey = container_of(key, HKey, node) ;

    if (znode -> len != hkey -> len) {
        return false ;
    }

    return 0 == memcmp(znode -> name, hkey -> name, znode -> len) ;
}

ZNode* zsetLookup(
    ZSet* zset, 
    const char* name, 
    size_t len
) {
    if (!zset -> root) {
        return NULL ;
    }

    HKey key ;
    key.node -> hash = strHash((uint8_t*)name, len) ;
    key.name = name ;
    key.len = len ;
    HNode* found = hmLookup(&zset -> hmap, key.node, &hcmp) ;

    return found? container_of(found, ZNode, hmap) : NULL ;
}

static bool zless(
    AVLNode* lhs, 
    double score,
    const char* name,
    size_t len
) {
    ZNode* zl = container_of(lhs, ZNode, tree) ;
    
    if (zl -> score != score) {
        return zl -> score < score ;
    }

    int rv = memcmp(zl -> name, name, std::min(zl -> len, len)) ;

    return (rv != 0)? (rv < 0) : (zl -> len < len) ;
}


static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree) ;
    return zless(lhs, zr -> score, zr -> name, zr -> len) ;
}


static void treeInsert(
    ZSet* zset, 
    ZNode* node
) {
    AVLNode* parent = NULL ;
    AVLNode** from = &zset -> root ;

    while (*from) {
        parent = *from ;
        from = zless(&node -> tree, parent)? &parent -> left : &parent -> right ;
    }

    *from = &node -> tree ;
    node -> tree.parent = parent ;
    zset -> root = avlFix(&node -> tree) ;
}


static void zsetUpdate(
    ZSet* zset, 
    ZNode* node, 
    double score
) {
    if (node -> score == score) {
        return ;
    }

    zset -> root = avlDelete(&node -> tree) ;
    avlInit(&node -> tree) ;

    node -> score = score ;
    treeInsert(zset, node) ;
}


bool zsetInsert(
    ZSet* zset, 
    const char* name, 
    size_t len, 
    double score
) {

    ZNode* node = zsetLookup(zset, name, len) ;

    if (node) {
        zsetUpdate(zset, node, score) ;
        return false ;
    }

    else {
        ZNode* node = znodeNew(name, len, score) ;
        hmInsert(&zset -> hmap, &node -> hmap) ;
        treeInsert(zset, node) ;

        return true ;
    }
}

void zsetDelete(
    ZSet* zset, 
    ZNode* node
) {
    HKey key ;
    key.node -> hash = node -> hmap.hash ;
    key.name = node -> name ;
    key.len = node -> len ;
    
    HNode* found = hmDelete(&zset -> hmap, key.node, &hcmp) ;
    assert(found) ;

    zset -> root = avlDelete(&node -> tree) ;
    znodeDelete(node) ;
}


static void treeDispose(AVLNode* node) {
    if (!node) return ;

    treeDispose(node -> left) ;
    treeDispose(node -> right) ;
    znodeDelete(container_of(node, ZNode, tree)) ;
}

void zsetClear(ZSet** zset) {
    hmClear(&(*zset) -> hmap) ;
    treeDispose((*zset) -> root) ;
    (*zset) -> root = NULL ;
}

ZNode *zsetSeekage(
    ZSet* zset, 
    double score, 
    const char* name, 
    size_t len
) {
    AVLNode* found = NULL ;

    for (AVLNode* node = zset -> root; node;) {
        if (zless(node, score, name, len)) {
            node = node -> right ;
        }

        else {
            found = node ;
            node = node -> left ;
        }
    }

    return found? container_of(found, ZNode, tree) : NULL ;
}



static AVLNode* successor(AVLNode* node) {
    if (node -> right) {
        for (node = node -> right; node -> left; node = node -> left) {}
        return node ;
    }

    while (AVLNode* parent = node -> parent) {
        if (node == parent -> left) {
            return parent ;
        }

        node = parent ;
    }

    return NULL ;
}



static AVLNode* predecessor(AVLNode* node) {
    if (node -> left) {
        for (node = node -> left; node -> right; node = node -> right) {}
        return node ;
    }

    while (AVLNode* parent = node -> parent) {
        if (node == parent -> right) {
            return parent ;
        }

        node = parent ;
    }

    return NULL ;
}


ZNode* znodeOffset(
    ZNode* node, 
    int64_t offset
) {
    AVLNode* tnode = node? avlOffset(&node -> tree, offset) : NULL ;
    return tnode? container_of(tnode, ZNode, tree) : NULL ;
}



static ZSet *expectZset(std::string &s) {
    LookupKey key ;
    key.key.swap(s) ;
    key.node.hash = strHash((uint8_t *)key.key.data(), key.key.size()) ;
    HNode *hnode = hmLookup(&g_data.db, &key.node, &entryEq) ;

    if (!hnode) {  
        return (ZSet *)&k_empty_zset ;
    }

    Entry *ent = container_of(hnode, Entry, node) ;
    return ent -> type == T_ZSET ? &ent -> zset : NULL ;
}

static void doZQuery(
    std::vector<std::string>& cmd, 
    Buffer& out
) {

    double score = 0 ;

    if (!str2Double(cmd[2], score)) {
        return outError(out, ERR_BAD_ARG, "expect float") ;
    }

    const std::string &name = cmd[3] ;
    int64_t offset = 0 ;
    int64_t limit = 0 ;

    if (!str2Int(cmd[4], offset) || str2Int(cmd[5], limit)) {
        return outError(out, ERR_BAD_ARG, "expect int") ;
    }

    ZSet* zset = expectZset(cmd[1]) ;

    if (!zset) {
        return outError(out, ERR_BAD_TYP, "expect zset");
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

    outEndArray(out, ctx, ((uint32_t)n));
}