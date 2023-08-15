#include "../include/config.h"
#include "node.cpp"
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
    // Node *root;
    BPTree(bool head_compression = false, bool tail_compression = false);
    ~BPTree();
    void insert(char *);
#ifdef TIME_DEBUG
    void insert_time(char *, double &, double &, double &, double &);
#endif
    int search(const char *);
#ifdef TOFIX
    int searchRange(string, string);
    int backwardScan(string, string);
#endif
    void getSize(Node *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(Node *);
    Node *getRoot();
    void printTree(Node *x, vector<bool> flag, bool compressed = true,
                   int depth = 0, bool isLast = false);

private:
    Node *_root;
    bool head_comp;
    bool tail_comp;
    void insert_leaf(Node *leaf, vector<Node *> &parents, char *key);
    void insert_nonleaf(Node *node, vector<Node *> &parents, int pos,
                        splitReturn_new childsplit);
    int search_insert_pos(Node *cursor, const char *key, int keylen, int low, int high,
                          bool &equal);
    int search_in_nonleaf(Node *cursor, const char *key, int keylen, int low, int high);
    int search_in_leaf(Node *cursor, const char *key, int keylen, int low, int high);
    Node *search_leaf_node(Node *root, const char *key, int keylen);
    Node *search_leaf_node_for_insert(Node *root, const char *key, int keylen,
                                      vector<Node *> &parents);
#ifdef DUPKEY
    bool check_split_condition(Node *node);
    int split_point(vector<Key_c> allkeys);
#else
    bool check_split_condition(Node *node, const char *key);
    int split_point(Node *node);
#endif
    splitReturn_new split_nonleaf(Node *node, vector<Node *> parents, int pos,
                                  splitReturn_new childsplit);
    splitReturn_new split_leaf(Node *node, vector<Node *> &parents, char *newkey);
    int get_child_pos_in_parent(Node *parent, Node *node);
    void get_node_bounds(vector<Node *> parents, int pos, Node *node,
                         string &lower_bound, string &upper_bound);
    vector<string> decompress_keys(Node *node, int pos = -1);
    int update_page_prefix(Node *node, char *page, vector<uint16_t> &idx,
                           int prefixlen, int low, int high);

#ifdef TIME_DEBUG
    void insert_leaf_time(Node *leaf, vector<Node *> &parents, char *key,
                          double &prefix_calc, double &prefix_update);
    splitReturn split_leaf_time(Node *node, vector<Node *> &parents, char *newkey,
                                double &prefix_calc, double &prefix_update);
#endif
};
#endif