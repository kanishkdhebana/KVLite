#include "heap.h"

static size_t heapLeftPos(size_t i) {
    return i * 2 + 1 ;
}

static size_t heapRightPos(size_t i) {
    return i * 2 + 2 ;
}

static size_t heapParentPos(size_t i) {
    return ((i + 1) / 2) - 1 ;
}

static void heapMoveUp(HeapItem* a, size_t pos) {
    HeapItem t = a[pos] ;

    while (pos > 0 && a[heapParentPos(pos)].val > t.val) {
        a[pos] = a[heapParentPos(pos)] ;
        *a[pos].ref = pos ; 
        pos = heapParentPos(pos) ;
    }

    a[pos] = t ;
    *a[pos].ref = pos ; 
}


static void heapMoveDown(HeapItem* a, size_t pos, size_t len) {
    HeapItem t = a[pos] ;
    
    while (true) {
        size_t left = heapLeftPos(pos) ;
        size_t right = heapRightPos(pos) ;
        
        size_t minPos = pos ;
        uint64_t minVal = t.val ;

        if (left < len && a[left].val < minVal) {
            minPos = left ;
            minVal = a[left].val ;
        }

        if (right < len && a[right].val < minVal) {
            minPos = right ;
        }

        if (minPos == pos) {
            break ;
        }

        a[pos] = a[minPos] ;
        pos = minPos ;
        *a[pos].ref = pos ; 
    }

    a[pos] = t ;
    *a[pos].ref = pos ; 
}


void heapUpdate(HeapItem* a, size_t pos, size_t len) {
    if (pos > 0 && a[heapParentPos(pos)].val < a[pos].val) {
        heapMoveUp(a, pos) ;
    }

    else {
        heapMoveDown(a, pos, len) ;
    }
}


void heapDelete(std:: vector<HeapItem>& a, size_t pos) {
    a[pos] = a.back() ;
    a.pop_back() ;
    
    if (pos < a.size()) {
        heapUpdate(a.data(), pos, a.size()) ;
    }
}

void heapInsert(std:: vector<HeapItem>& a, size_t pos, HeapItem t) {
    if (pos < a.size()) {
        a[pos] = t ;
    }

    else {
        pos = a.size() ;
        a.push_back(t) ;
    }

    heapUpdate(a.data(), pos, a.size()) ;
}
