#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "node_mt.cpp"
#include "util.cpp"
#include "../include/config.h"
#include "./compression/compression_pkb.cpp"

using namespace std;

// BP tree for PKB compression
class BPTreePkBMT{
public:
	BPTreePkBMT();
	~BPTreePkBMT();
	unordered_map<string, char *> strPtrMap;
	int search(string_view);
	int searchRange(string, string);
	int backwardScan(string, string);
	void insert(string);
	void getSize(NodePkB *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(NodePkB *);
	NodePkB *getRoot();
	void setRoot(NodePkB *);
	void lockRoot(accessMode mode);
	void printTree(NodePkB *x, vector<bool> flag, bool compressed = false,
				   int depth = 0, bool isLast = false);
private:
	NodePkB *root;
	bool head_comp;
	bool tail_comp;
	void insert_leaf(NodePkB *leaf, vector<NodePkB *> &parents, string key, char *keyptr, int offset);
	void insert_nonleaf(NodePkB *node, vector<NodePkB *> &parents, int pos, splitReturnPkB childsplit);
	NodePkB* search_leaf_node(NodePkB *root, string_view key, vector<NodePkB *> &parents, int &offset);
	bool check_split_condition(NodePkB *node);
	int split_point(vector<KeyPkB> allkeys);
	uncompressedKey get_uncompressed_key_before_insert(NodePkB *node, int ind, int insertpos, string newkey, char *newkeyptr, bool equal);
	splitReturnPkB split_nonleaf(NodePkB *node, vector<NodePkB *> parents, int pos, splitReturnPkB childsplit, int offset);
	splitReturnPkB split_leaf(NodePkB *node, vector<NodePkB *> &parents, string newkey, char *newkeyptr, int offset);
	void bt_fetch_root(NodePkB *currRoot, vector<NodePkB *> &parents);
	NodePkB* scan_node(NodePkB *node, string key,int &offset);
	NodePkB* move_right(NodePkB *node, string key,int &offset);
	vector<string> decompress_keys(NodePkB *node, int pos = -1);
};
