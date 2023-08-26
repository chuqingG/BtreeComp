#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node_mt.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeMyISAMMT{
public:
	// NodeMyISAM *root;
	BPTreeMyISAMMT(bool non_leaf_comp=false);
	~BPTreeMyISAMMT();
	int search(string_view);
	void insert(string);
	void getSize(NodeMyISAM *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(NodeMyISAM *);
	NodeMyISAM *getRoot();
	void setRoot(NodeMyISAM *);
	void lockRoot(accessMode mode);
	void printTree(NodeMyISAM *x, vector<bool> flag, bool compressed = false,
				   int depth = 0, bool isLast = false);
private:
	NodeMyISAM *root;
    bool non_leaf_comp;
	int prefix_insert(NodeMyISAM *cursor, string_view key, bool &equal);
	int prefix_search(NodeMyISAM *node, string_view key);
	void insert_leaf(NodeMyISAM *leaf, vector<NodeMyISAM *> &parents, string key);
	void insert_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, int pos, splitReturnMyISAM childsplit);
	int insert_binary(NodeMyISAM *cursor, string_view key, int low, int high, bool &equal);
	int search_binary(NodeMyISAM *cursor, string_view key, int low, int high);
	NodeMyISAM* search_leaf_node(NodeMyISAM *root, string_view key, vector<NodeMyISAM *> &parents);
	bool check_split_condition(NodeMyISAM *node);
	int split_point(vector<KeyMyISAM> allkeys);
	splitReturnMyISAM split_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> parents, int pos, splitReturnMyISAM childsplit);
	splitReturnMyISAM split_leaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, string newkey);
	void bt_fetch_root(NodeMyISAM *currRoot, vector<NodeMyISAM *> &parents);
	NodeMyISAM* scan_node(NodeMyISAM *node, string key);
	NodeMyISAM* move_right(NodeMyISAM *node, string key);
};
