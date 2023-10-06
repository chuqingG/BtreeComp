#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
#include "node_inline.h"
#include "../utils/compare.cpp"
#include "../utils/config.h"

using namespace std;

// BP tree
class BPTreeWT {
public:
    BPTreeWT(bool non_leaf_compression, bool suffix_compression = true);
    ~BPTreeWT();
    int search(const char *);
    void insert(char *);
    int searchRange(const char *min, const char *max);
    void getSize(NodeWT *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodeWT *);
    NodeWT *getRoot();
    void printTree(NodeWT *x, vector<bool> flag, bool compressed = false,
                   int depth = 0, bool isLast = false);

private:
    NodeWT *_root;
    bool non_leaf_comp;
    bool suffix_comp;
    int max_level;
    void insert_leaf(NodeWT *leaf, NodeWT **path, int path_level, char *key, int keylen);
    void insert_nonleaf(NodeWT *node, NodeWT **path, int pos, splitReturnWT *childsplit);
    // insert_binary
    int search_insert_pos(NodeWT *cursor, const char *key, int keylen,
                          int low, int high, uint16_t &skiplow, bool &equal);
    // search_binary
    int search_in_leaf(NodeWT *cursor, const char *key, int keylen, int low, int high, uint16_t &skiplow);
    NodeWT *search_leaf_node(NodeWT *root, const char *key, int keylen, uint16_t &skiplow);
    NodeWT *search_leaf_node_for_insert(NodeWT *root, const char *key, int keylen,
                                        NodeWT **path, int &path_level, uint16_t &skiplow);
    bool check_split_condition(NodeWT *node, int keylen);
    int split_point(NodeWT *node);
    splitReturnWT split_nonleaf(NodeWT *node, int pos, splitReturnWT *childsplit);
    splitReturnWT split_leaf(NodeWT *node, char *newkey, int newkey_len);
};