#include "../include/config.h"
#include "node.cpp"
#include "node_inline.h"
#include "util.cpp"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
// #include "../include/util.h"

using namespace std;

#ifndef __TREE_H__
#define __TREE_H__

// BP tree
class BPTree {
public:
    BPTree(bool head_compression = false, bool tail_compression = false);
    ~BPTree();
    void insert(char *);
    int search(const char *);
    void getSize(Node *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(Node *);
    Node *getRoot();
    void printTree(Node *x, vector<bool> flag, bool compressed = true,
                   int depth = 0, bool isLast = false);

private:
    Node *_root;
    bool head_comp;
    bool tail_comp;
    int max_level;
    void insert_leaf(Node *leaf, Node **path, int path_level, char *key, int keylen);
    void insert_nonleaf(Node *node, Node **path, int pos,
                        splitReturn_new *childsplit);
    int search_insert_pos(Node *cursor, const char *key, int keylen, int low, int high,
                          bool &equal);
    int search_in_node(Node *cursor, const char *key, int keylen, int low, int high, bool isleaf);
    Node *search_leaf_node(Node *root, const char *key, int keylen);
    Node *search_leaf_node_for_insert(Node *root, const char *key, int keylen,
                                      Node **path, int &path_level);
#ifdef DUPKEY
    bool check_split_condition(Node *node);
    int split_point(vector<Key_c> allkeys);
#else
    bool check_split_condition(Node *node, int keylen);
    int split_point(Node *node);
#endif
    splitReturn_new split_nonleaf(Node *node, int pos,
                                  splitReturn_new *childsplit);
    splitReturn_new split_leaf(Node *node, char *newkey, int newkey_len);
};
#endif