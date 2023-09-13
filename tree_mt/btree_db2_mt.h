#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "node_mt.cpp"
#include "./compression/compression_db2.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeDB2MT {
public:
    BPTreeDB2MT();
    ~BPTreeDB2MT();
    int search(string_view, bool print = false);
    void insert(string);
    void getSize(NodeDB2 *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodeDB2 *);
    void setRoot(NodeDB2 *);
    NodeDB2 *getRoot();
    void lockRoot(accessMode mode);
    void printTree(NodeDB2 *x, vector<bool> flag, bool compressed = false,
                   int depth = 0, bool isLast = false);

private:
    NodeDB2 *root;
    void insert_leaf(NodeDB2 *leaf, vector<NodeDB2 *> &parents, string key);
    void insert_nonleaf(NodeDB2 *node, vector<NodeDB2 *> &parents, int pos, splitReturnDB2 childsplit);
    static int insert_binary(NodeDB2 *cursor, string_view key, int low, int high, bool &equal);
    int search_binary(NodeDB2 *cursor, string_view key, int low, int high);
    NodeDB2 *search_leaf_node(NodeDB2 *root, string_view key, vector<NodeDB2 *> &parents, bool print = false);
    bool check_split_condition(NodeDB2 *node);
    int split_point(vector<Key> allkeys); //
    splitReturnDB2 split_nonleaf(NodeDB2 *node, vector<NodeDB2 *> parents, int pos, splitReturnDB2 childsplit);
    splitReturnDB2 split_leaf(NodeDB2 *node, vector<NodeDB2 *> &parents, string newkey);
    void bt_fetch_root(NodeDB2 *currRoot, vector<NodeDB2 *> &parents);
    NodeDB2 *scan_node(NodeDB2 *node, string key, bool print = false);
    NodeDB2 *move_right(NodeDB2 *node, string key);
};