#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>

#include "node.cpp"
#include "node_inline.h"
#include "./compression/compression_db2.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#ifndef __DB2_H__
#define __DB2_H__

// BP tree
class BPTreeDB2 {
public:
    BPTreeDB2();
    ~BPTreeDB2();
    int search(const char *);
    void insert(char *);

    void getSize(NodeDB2 *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodeDB2 *);
    NodeDB2 *getRoot();
    void printTree(NodeDB2 *x, vector<bool> flag, bool compressed = true,
                   int depth = 0, bool isLast = false);

private:
    NodeDB2 *_root;
    int max_level;
    void insert_leaf(NodeDB2 *leaf, NodeDB2 **path, int path_level, char *key, int keylen);
    void insert_nonleaf(NodeDB2 *node, NodeDB2 **path, int pos, splitReturnDB2 *childsplit);
    // insert_binary
    static int search_insert_pos(NodeDB2 *cursor, const char *key, int keylen, int low, int high, bool &equal);
    int insert_prefix_and_key(NodeDB2 *node, const char *key, int keylen, bool &equal);
    // search_binary
    int search_in_leaf(NodeDB2 *cursor, const char *key, int keylen, int low, int high);
    NodeDB2 *search_leaf_node(NodeDB2 *root, const char *key, int keylen);
    NodeDB2 *search_leaf_node_for_insert(NodeDB2 *root, const char *key, int keylen,
                                         NodeDB2 **path, int &path_level);

    bool check_split_condition(NodeDB2 *node, int keylen);
    int split_point(NodeDB2 *node); //

    splitReturnDB2 split_nonleaf(NodeDB2 *node, int pos, splitReturnDB2 *childsplit);
    splitReturnDB2 split_leaf(NodeDB2 *node, char *newkey, int newkey_len);
    void do_split_node(NodeDB2 *node, NodeDB2 *right, int splitpos, bool isleaf, Item &splitprefix);
};
#endif