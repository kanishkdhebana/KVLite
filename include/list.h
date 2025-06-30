#ifndef DOUBLY_LL_H
#define DOUBLY_LL_H


struct DList {
    DList* prev = NULL ;
    DList* next = NULL ;
} ;


inline void dlistInit(DList* node) {
    node -> prev = node ;
    node -> next = node ;
}


inline bool dlistEmpty(DList* node) {
    return node -> next == node ;
}


inline void dlistInsertBefore(DList* target, DList* newNode) {
    DList* prev = target -> prev ;
    prev -> next = newNode ;
    newNode -> prev = prev ;
    newNode -> next = target ;
    target -> prev = newNode ;
}


inline void dlistDetach(DList* node) {
    DList* prev = node -> prev ;
    DList* next = node -> next ;
    prev -> next = next ;
    next -> prev = prev ;
}


#endif