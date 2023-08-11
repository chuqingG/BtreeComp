#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "node_mt_mc.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeMT{
public:
	// Node *root;
	BPTreeMT(int col, bool head_compression, bool tail_compression);
	~BPTreeMT();
	// D
	int search(string_view);
	// D
	int searchRange(string, string);
	void insert(string);
	void getSize(Node *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(Node *);
	Node *getRoot();
	void setRoot(Node *);
	void lockRoot(accessMode mode);
	void printTree(Node *x, vector<bool> flag, bool compressed = true,
				   int depth = 0, bool isLast = false);
private:
	Node *root;
	bool head_comp;
	bool tail_comp;
	int column_num;
	// unfinish
	void insert_leaf(Node *leaf, vector<Node *> &parents, string key);
	//
	void insert_nonleaf(Node *node, vector<Node *> &parents, int pos, splitReturn childsplit);
	//
	int insert_binary(Node *cursor, string_view key, int low, int high);
	int search_binary(Node *cursor, string_view key, int low, int high);
	//
	Node* search_leaf_node(Node *root, string_view key, vector<Node *> &parents);
	//
	bool check_split_condition(Node *node);
	int split_point(vector<string> allkeys);
	//
	splitReturn split_nonleaf(Node *node, vector<Node *> parents, int pos, splitReturn childsplit);
	//
	splitReturn split_leaf(Node *node, vector<Node *> &parents, string newkey);
	void bt_fetch_root(Node *currRoot, vector<Node *> &parents);
	//
	Node* scan_node(Node *node, string_view key);
	//
	Node* move_right(Node *node, string key);

};