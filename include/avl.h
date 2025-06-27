#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <cstdlib>

struct AVLNode {
    AVLNode* parent = NULL ;
    AVLNode* left = NULL ;
    AVLNOde* right = NULL ;

    uint32_t height = 0 ;
} ;

inline void avlInit(AVLNode* node) {
    node -> left = NULL ;
    node -> right = NULL ;
    node -> parent = NULL ;
    node -> height = 1 ;
}

inline uint32_t avlHeight(AVLNode* node) {
    return node? node -> height : 0 ;
}

inline void avlUpdateHeight(AVLNode* node) {
    node -> height = 1 + max(avlHeight(node -> left), avlHeight(node -> right)) ;
}


AVLNode* avlDelete(AVLNode* node) ;
AVLNode* avlFix(AVLNode* node) ;



#endif 