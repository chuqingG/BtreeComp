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
class BPTreeDB2MT{
public:
	BPTreeDB2MT();
	~BPTreeDB2MT();
	int search(string_view, bool print=false);
	int searchRange(string, string);
	int backwardScan(string, string);
	void insert(string);
	void getSize(DB2Node *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(DB2Node *);
	void setRoot(DB2Node *);
	DB2Node *getRoot();
	void lockRoot(accessMode mode);
	void printTree(DB2Node *x, vector<bool> flag, bool compressed = false,
				   int depth = 0, bool isLast = false);

private:
	DB2Node *root;
	void insert_leaf(DB2Node *leaf, vector<DB2Node *> &parents, string key);
	void insert_nonleaf(DB2Node *node, vector<DB2Node *> &parents, int pos, splitReturnDB2 childsplit);
	static int insert_binary(DB2Node *cursor, string_view key, int low, int high, bool &equal);
	int search_binary(DB2Node *cursor, string_view key, int low, int high);
	DB2Node* search_leaf_node(DB2Node *root, string_view key, vector<DB2Node *> &parents, bool print = false);
	bool check_split_condition(DB2Node *node);
	int split_point(vector<Key> allkeys); //
	splitReturnDB2 split_nonleaf(DB2Node *node, vector<DB2Node *> parents, int pos, splitReturnDB2 childsplit);
	splitReturnDB2 split_leaf(DB2Node *node, vector<DB2Node *> &parents, string newkey);
	void bt_fetch_root(DB2Node *currRoot, vector<DB2Node *> &parents);
	DB2Node* scan_node(DB2Node *node, string key, bool print=false);
	DB2Node* move_right(DB2Node *node, string key);
	vector<string> decompress_keys(DB2Node *node, int pos = -1);
};