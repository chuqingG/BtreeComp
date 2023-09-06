#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
#include "node_inline.h"
#include "./compression/compression_db2.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

#ifndef __DB2_H__
#define __DB2_H__

// BP tree
class BPTreeDB2 {
public:
    BPTreeDB2();
    ~BPTreeDB2();
    int search(const char *);
    void insert(char *);

    void getSize(DB2Node *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(DB2Node *);
    DB2Node *getRoot();
    void printTree(DB2Node *x, vector<bool> flag, bool compressed = true,
                   int depth = 0, bool isLast = false);

private:
    DB2Node *_root;
    int max_level;
    void insert_leaf(DB2Node *leaf, DB2Node **path, int path_level, char *key, int keylen);
    void insert_nonleaf(DB2Node *node, DB2Node **path, int pos, splitReturnDB2 childsplit);
    // insert_binary
    static int search_insert_pos(DB2Node *cursor, const char *key, int keylen, int low, int high, bool &equal);
    int insert_prefix_and_key(DB2Node *node, const char *key, int keylen, bool &equal);
    // search_binary
    int search_in_leaf(DB2Node *cursor, const char *key, int keylen, int low, int high);
    DB2Node *search_leaf_node(DB2Node *root, const char *key, int keylen);
    DB2Node *search_leaf_node_for_insert(DB2Node *root, const char *key, int keylen,
                                         DB2Node **path, int &path_level);

    bool check_split_condition(DB2Node *node, int keylen);
    int split_point(DB2Node *node); //

    splitReturnDB2 split_nonleaf(DB2Node *node, int pos, splitReturnDB2 childsplit);
    splitReturnDB2 split_leaf(DB2Node *node, char *newkey, int newkey_len);
    Data *do_split_node(DB2Node *node, DB2Node *right, int splitpos, bool isleaf);
};
#endif