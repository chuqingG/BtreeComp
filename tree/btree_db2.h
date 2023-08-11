#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
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
#ifdef TOFIX
    int searchRange(string, string);
    int backwardScan(string, string);
#endif
    void getSize(DB2Node *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(DB2Node *);
    DB2Node *getRoot();
    void printTree(DB2Node *x, vector<bool> flag, bool compressed = true,
                   int depth = 0, bool isLast = false);

private:
    DB2Node *_root;
    void insert_leaf(DB2Node *leaf, vector<DB2Node *> &parents, char *key);
    void insert_nonleaf(DB2Node *node, vector<DB2Node *> &parents, int pos, splitReturnDB2 childsplit);
    static int insert_binary(DB2Node *cursor, const char *key, int low, int high, bool &equal);
    int search_binary(DB2Node *cursor, const char *key, int low, int high);
    DB2Node *search_leaf_node(DB2Node *root, const char *key, vector<DB2Node *> &parents);

    bool check_split_condition(DB2Node *node);
    int split_point(vector<Key_c> allkeys); //

    splitReturnDB2 split_nonleaf(DB2Node *node, vector<DB2Node *> parents, int pos, splitReturnDB2 childsplit);
    splitReturnDB2 split_leaf(DB2Node *node, vector<DB2Node *> &parents, char *newkey);
#ifdef TOFIX
    vector<string> decompress_keys(DB2Node *node, int pos = -1);
#endif
};
#endif