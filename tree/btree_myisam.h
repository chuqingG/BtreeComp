#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
#include "node_inline.h"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeMyISAM {
public:
    // NodeMyISAM *root;
    BPTreeMyISAM(bool non_leaf_comp = false);
    ~BPTreeMyISAM();
    int search(const char *);
    void insert(char *);
    void getSize(NodeMyISAM *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodeMyISAM *);
    NodeMyISAM *getRoot();
    void printTree(NodeMyISAM *x, vector<bool> flag, bool compressed = false,
                   int depth = 0, bool isLast = false);

private:
    NodeMyISAM *_root;
    bool non_leaf_comp;
    int max_level;
    int prefix_insert(NodeMyISAM *cursor, const char *key, int keylen, bool &equal);
    int prefix_search(NodeMyISAM *node, const char *key, int keylen);
    void insert_leaf(NodeMyISAM *leaf, NodeMyISAM **path, int path_level, char *key, int keylen);
    void insert_nonleaf(NodeMyISAM *node, NodeMyISAM **path, int pos, splitReturnMyISAM *childsplit);
    // insert_binary
    int search_insert_pos(NodeMyISAM *cursor, const char *key, int keylen, int low, int high, bool &equal);
    NodeMyISAM *search_leaf_node(NodeMyISAM *searchroot, const char *key, int keylen);
    NodeMyISAM *search_leaf_node_for_insert(NodeMyISAM *searchroot, const char *key, int keylen,
                                            NodeMyISAM **path, int &path_level);
    bool check_split_condition(NodeMyISAM *node, int keylen);
    int split_point(NodeMyISAM *node);
    splitReturnMyISAM split_nonleaf(NodeMyISAM *node, int pos, splitReturnMyISAM *childsplit);
    splitReturnMyISAM split_leaf(NodeMyISAM *node, char *newkey, int newkey_len);
};
