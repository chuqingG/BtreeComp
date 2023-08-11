#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "node.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeMyISAM{
public:
	// NodeMyISAM *root;
	BPTreeMyISAM(bool non_leaf_comp=false);
	~BPTreeMyISAM();
	int search(string_view);
	int searchRange(string, string);
	int backwardScan(string, string);
	void insert(string);
	void getSize(NodeMyISAM *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(NodeMyISAM *);
	NodeMyISAM *getRoot();
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
	vector<string> decompress_keys(NodeMyISAM *node, int pos = -1);
};
