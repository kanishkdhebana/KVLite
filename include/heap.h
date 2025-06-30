#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

struct HeapItem {
    uint64_t val ;
    size_t* ref ;
} ;

void heapUpdate(HeapItem* a, size_t pos, size_t len) ;
void heapDelete(std:: vector<HeapItem>& a, size_t pos) ;
void heapInsert(std:: vector<HeapItem>& a, size_t pos, HeapItem t) ;

#endif