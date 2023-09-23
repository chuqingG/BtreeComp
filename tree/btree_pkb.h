#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
#include "node_inline.h"
#include "util.cpp"
#include "../include/config.h"
#include "./compression/compression_pkb.cpp"

using namespace std;

// BP tree
class BPTreePkB {
public:
    BPTreePkB();
    ~BPTreePkB();
    int search(const char *);
    void insert(char *);
    int searchRange(const char *, const char *);

    void getSize(NodePkB *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodePkB *);
    NodePkB *getRoot();
    void printTree(NodePkB *x, vector<bool> flag, bool compressed = false,
                   int depth = 0, bool isLast = false);

private:
    NodePkB *_root;
    int max_level;
    void insert_leaf(NodePkB *leaf, NodePkB **path, int path_level, char *key, int keylen, int offset);
    void insert_nonleaf(NodePkB *node, NodePkB **path, int pos, splitReturnPkB *childsplit);
    NodePkB *search_leaf_node(NodePkB *root, const char *key, int keylen, int &offset);
    NodePkB *search_leaf_node_for_insert(NodePkB *root, const char *key, int keylen,
                                         NodePkB **path, int &path_level, int &offset);
    bool check_split_condition(NodePkB *node, int keylen);
    int split_point(NodePkB *node);
    // uncompressedKey get_uncompressed_key_before_insert(NodePkB *node, int ind, int insertpos, string newkey, char *newkeyptr, bool equal);
    splitReturnPkB split_nonleaf(NodePkB *node, NodePkB **path, int parentlevel, splitReturnPkB *childsplit, int offset);
    splitReturnPkB split_leaf(NodePkB *node, NodePkB **path, int path_level, char *newkey, int newkey_len, int offset);
};
