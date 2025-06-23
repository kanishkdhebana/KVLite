#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include <stdint.h>


const size_t k_max_load_factor = 8 ;
const size_t k_rehashing_work = 128 ; 

struct HNode {
    HNode* next = NULL ;
    uint64_t hash = 0 ;
} ;

struct HTable {
    HNode** table = NULL ;
    size_t mask = 0 ; // power of 2 size of table
    size_t size = 0 ; // Number of keys in the table
} ;

struct HMap {
    HTable newer ;
    HTable older ;
    size_t migratePosition = 0 ;
} ;

void hmTriggerRehashing(HMap* hmap) ;
void hmHelpRehashing(HMap* hmap) ;
HNode* hmLookup(HMap* hmap,HNode* key,bool (*equal)(const HNode*, const HNode*)) ;
void hmInsert(HMap* hmap,HNode* node) ;
HNode* hmDelete(HMap* hmap,HNode* key,bool (*equals)(const HNode*, const HNode*)) ;

#endif