#include <assert.h>
#include <stdlib.h> 
#include <cstdio>  
#include "hashtable.h"


static void htInit(
    HTable* hTable, 
    size_t size
) {
    assert (size > 0 && (size & (size - 1)) == 0) ; // size must be a power of 2
    hTable -> mask = size - 1 ;
    hTable -> size = 0 ;
    hTable -> table = (HNode**)calloc(size, sizeof(HNode*)) ;

    if (!hTable -> table) {
        perror("calloc") ;
        exit(EXIT_FAILURE) ;
    }
}


static void htInsert(
    HTable* hTable, 
    HNode* node
) {
    size_t pos = (node -> hash) & (hTable -> mask) ; // as mask is (111...111) in binary, '&' will effectively be a power of 2, this is equivalent to hash % size
    node -> next = hTable -> table[pos] ;
    hTable -> table[pos] = node ;
    hTable -> size++ ;
}


static HNode* htDetach(
    HTable* hTable,
    HNode** from 
) {
    HNode* node = *from ;

    if (node == NULL) {
    fprintf(stderr, "htDetach error: tried to detach NULL node! from = %p\n", (void*)from);
    abort();
}

    *from = node -> next ;
    hTable -> size-- ;
    
    return node ;
}


static HNode** htLookup(
    HTable* hTable, 
    HNode* key, 
    bool (*equal)(const HNode*, const HNode*)
) {
    if (!hTable || !hTable -> table) {
        return NULL ;
    }

    size_t pos = (key -> hash) & (hTable -> mask) ;
    HNode** from = &hTable -> table[pos] ;

    for (HNode* curr = *from; curr != NULL; curr = curr -> next) {
        if (curr -> hash == key -> hash && equal(curr, key)) {
            return from ;
        }
    }

    return NULL ;
}


void hmTriggerRehashing(HMap* hmap) {
    hmap -> older = hmap -> newer ;
    htInit(&hmap -> newer, (hmap -> newer.mask + 1) * 2) ;
    hmap -> migratePosition = 0 ;
}


void hmHelpRehashing(HMap* hmap) {
    size_t nWork = 0 ;

    while (nWork < k_rehashing_work && hmap -> older.size > 0) {
        HNode** from = &hmap -> older.table[hmap -> migratePosition] ;

        // if *from is a nullptr, skip this position
        if (!*from) {
            hmap -> migratePosition++ ;
            continue ;
        }

        htInsert(&hmap -> newer, htDetach(&hmap -> older, from)) ;
        nWork++ ;
    }

    if (hmap -> older.size == 0 && hmap -> older.table) {
        free(hmap -> older.table) ;
        hmap -> older = HTable{} ;
    }
}


HNode* hmLookup(
    HMap* hmap,
    HNode* key,
    bool (*equal)(const HNode*, const HNode*)
) {
    hmHelpRehashing(hmap) ;

    HNode** from = htLookup(&hmap -> newer, key, equal) ;

    if (!from) {
        from = htLookup(&hmap -> older, key, equal) ;
    }

    return from? *from : NULL ;
}


void hmInsert(
    HMap* hmap,
    HNode* node
) {
    if (!hmap -> newer.table) {
        htInit(&hmap -> newer, 4) ;
    }

    htInsert(&hmap -> newer, node) ;

    if (!hmap -> older.table) {
        size_t threshold = (hmap -> newer.mask + 1) * k_max_load_factor ;

        if (hmap -> newer.size >= threshold) {
            hmTriggerRehashing(hmap) ;
        }
    }

    hmHelpRehashing(hmap) ;
}


HNode* hmDelete(
    HMap* hmap,
    HNode* key,
    bool (*equal)(const HNode*, const HNode*)
) {
    hmHelpRehashing(hmap) ;

    if (HNode** from = htLookup(&hmap -> newer, key, equal)) {
        return htDetach(&hmap -> newer, from) ;
    }

    if (HNode** from = htLookup(&hmap -> older, key, equal)) {
        return htDetach(&hmap -> older, from) ;
    }

    return NULL ;
}


void hmClear(HMap* hmap) {
    free(hmap -> newer.table) ;
    free(hmap -> older.table) ;
    *hmap = HMap{} ;    
}


size_t hmSize(HMap* hmap) {
    return hmap -> newer.size + hmap -> older.size ;
}

static bool htForEach(HTable* htable, bool (*f)(HNode*, void*), void* arg) {
    for (size_t i = 0; htable -> mask != 0 && i <= htable -> mask; i++) {
        for (HNode* node = htable -> table[i]; node != NULL; node = node -> next) {
            if (!f(node, arg)) {
                return false ;
            }
        }
    }

    return true ;
}


void hmForEach(HMap* hmap, bool (*f)(HNode*, void*), void* arg) {
    htForEach(&hmap -> newer, f, arg) && htForEach(&hmap -> older, f, arg) ;
}