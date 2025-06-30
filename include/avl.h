#ifndef AVL_H
#define AVL_H

#include <cstdint>
#include <cstdlib>
#include <algorithm>

struct AVLNode {
    AVLNode* parent = NULL ;
    AVLNode* left = NULL ;
    AVLNode* right = NULL ;

    uint32_t height = 0 ;
    uint32_t cnt = 0 ; // subtree size
} ;

inline void avlInit(AVLNode* node) {
    node -> left = NULL ;
    node -> right = NULL ;
    node -> parent = NULL ;
    node -> height = 1 ;
}

inline uint32_t avlCnt(AVLNode* node) {
    return node? node -> cnt : 0 ;
}

inline uint32_t avlHeight(AVLNode* node) {
    return node? node -> height : 0 ;
}

inline void avlUpdateHeight(AVLNode* node) {
    node -> height = 1 + std::max(avlHeight(node -> left), avlHeight(node -> right)) ;
    node -> cnt = 1 + avlCnt(node -> left) + avlCnt(node -> right) ;
}


AVLNode* avlDelete(AVLNode* node) ;
AVLNode* avlFix(AVLNode* node) ;
AVLNode* avlOffset(AVLNode* node, int64_t offset) ;


#endif 