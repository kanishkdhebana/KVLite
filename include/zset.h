#ifndef ZSET_H
#define ZSET_H

#include "avl.h"
#include "hashtable.h"


 struct ZSet {
    AVLNode* root = NULL ; // indexed by (score, name)
    HMap hmap ; // indexed by name
} ;

struct ZNode {
    AVLNode tree ;
    HNode hmap ;
    double score = 0 ;
    size_t len = 0 ;    
    char name[0] ;
} ;

struct HKey {
    HNode node ;
    const char* name = NULL ;
    size_t len = 0 ;
} ;

bool zsetInsert(ZSet* zset, const char* name, size_t len, double score) ;
ZNode* zsetLookup(ZSet* zset, const char* name, size_t len) ;
void zsetDelete(ZSet* zset, ZNode* node) ;
void zsetClear(ZSet* zset) ;
ZNode* znodeOffset(ZNode* node, int64_t offset) ;
ZNode* zsetSeekage(ZSet *zset, double score, const char *name, size_t len) ;

#endif 