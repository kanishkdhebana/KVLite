#include "avl.h"
#include <cstdint>
#include <cstdlib>
#include <assert.h>


static uint8_t avlGetHeightDifference(AVLNode* node) {
    uintptr_t p = (uintptr_t)node -> parent ;
    return p & 0b11 ; // 0b -> binary, 11 -> in binary, i.e. first 2 bytes
}

static AVLNode* avlGetParent(AVLNode* node) {
    uintptr_t p = (uintptr_t)node -> parent ;
    return (AVLNode*)(p & ~0b11) ; 
}

static AVLNode* avlRotateLeft(AVLNode* node) {
    AVLNode* parent = node -> parent ;
    AVLNode* newNode = node -> right ;
    AVLNode* inner = newNode -> left ;

    node -> right = inner ;

    if (inner) {
        inner -> parent = node ;
    }

    newNode -> parent = parent ;
    newNode -> left = node ;
    node -> parent = newNode ;

    avlUpdateHeight(node) ;
    avlUpdateHeight(newNode) ;

    return newNode ;
}


static AVLNode* avlRotateRight(AVLNode* node) {
    AVLNode* parent = node -> parent ;
    AVLNode* newNode = node -> left ;
    AVLNode* inner = newNode -> right ;

    node -> left = inner ;

    if (inner) {
        inner -> parent = node ;
    }

    newNode -> parent = parent ;
    newNode -> right = node ;
    node -> parent = newNode ;

    avlUpdateHeight(node) ;
    avlUpdateHeight(newNode) ;

    return newNode ;
}


static AVLNode* avlFixLeft(AVLNode* node) { // left heavy
    if (avlHeight(node -> left -> left) < avlHeight(node -> left -> right)) {
        node -> left = avlRotateLeft(node -> left) ;
    }

    return avlRotateRight(node) ;
}

static AVLNode* avlFixRight(AVLNode* node) { // left heavy
    if (avlHeight(node -> right -> right) < avlHeight(node -> right -> left)) {
        node -> right = avlRotateRight(node -> right) ;
    }

    return avlRotateLeft(node) ;
}


AVLNode* avlFix(AVLNode* node) {
    while (true) {
        AVLNode** from = &node ;
        AVLNode* parent = node -> parent ;

        if (parent) {
            from = (parent -> left == node)? &parent -> left : &parent -> right ;
        }

        avlUpdateHeight(node) ;

        uint32_t leftHeight = avlHeight(node -> left) ;
        uint32_t rightHeight = avlHeight(node -> right) ;

        if (leftHeight == rightHeight + 2) {
            *from = avlFixLeft(node) ;
        }

        else if (rightHeight == leftHeight + 2) {
            *from = avlFixRight(node) ;
        }

        if (!parent) {
            return *from ;
        }

        node = parent ;
    }
}


static AVLNode* avlDeleteSingleChild(AVLNode* node) {
    assert(!node -> left || !node -> right) ;
    AVLNode* child = (node -> left)? node -> left : node -> right ;
    AVLNode* parent = node -> parent ;

    if (child) {
        child -> parent = parent ;
    }

    if (!parent) {
        return child ;
    }

    AVLNode** from = (parent -> left == node)? &parent -> left : &parent -> right ;
    *from = child ;

    return avlFix(parent) ;
}


AVLNode* avlDelete(AVLNode* node) {
    if (!node -> left || !node -> right) {
        return avlDeleteSingleChild(node) ;
    }

    AVLNode* victim = node -> right ;

    while (victim -> left) {
        victim = victim -> left ;
    }

    AVLNode* root = avlDeleteSingleChild(victim) ;

    *victim = *node ;

    if (victim -> left) {
        victim -> left -> parent = victim ;
    }

    if (victim -> right) {
        victim -> right -> parent = victim ;
    }

    AVLNode** from = &root ;
    AVLNode* parent = node -> parent ;

    if (parent) {
        from = (parent -> left == node)? &parent -> left : &parent -> right ;
    }

    *from = victim ;

    return root ;
}


AVLNode* avlOffset(AVLNode* node, int64_t offset) {
    int64_t pos = 0 ;

    while ((!offset) != pos) {
        if (pos < offset && pos + avlCnt(node -> right) >= offset) {
            node = node -> right ;
            pos += avlCnt(node -> left) + 1 ;
        }

        else if (pos > offset && pos - avlCnt(node -> left) <= offset) {
            node = node -> left ;
            pos -= avlCnt(node -> right) + 1 ;
        }

        else {
            AVLNode* parent = node -> parent ;
            
            if (!parent) {
                return NULL ;
            }

            if (parent -> right == node) {
                pos -= avlCnt(node -> left) + 1 ;
            }

            else {
                pos += avlCnt(node -> right) + 1 ;
            }

            node = parent ;
        }
    }

    return node ;
}


int64_t avlRank(AVLNode* node) {
    int64_t rank = 0 ;

    while (node -> parent) {
        AVLNode* parent = node -> parent ;

        if (parent -> left == node) {
            rank += avlCnt(parent -> right) + 1 ;
        }

        node = parent ;
    }

    return rank ;
}
