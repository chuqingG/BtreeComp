#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "node_mt.cpp"
#include "util.cpp"
#include "../include/config.h"

using namespace std;

// BP tree
class BPTreeMT{
public:
	BPTreeMT(bool head_compression=false, bool tail_compression=false);
	~BPTreeMT();
	int search(const char*, bool print=false);
#ifndef CHARALL
	int searchRange(string, string);
	int backwardScan(string, string);
#endif
	void insert(char*);
#ifdef TIMEDEBUG
	void insert_time(char*, double*, double*);
#endif
	void getSize(Node *, int &, int &, int &, int &, unsigned long &, int &);
	int getHeight(Node *);
	Node *getRoot();
	void setRoot(Node *);
	void lockRoot(accessMode mode);
	void printTree(Node *x, vector<bool> flag, bool compressed = false,
				   int depth = 0, bool isLast = false);
private:
	Node *root;
	bool head_comp;
	bool tail_comp;
	
	void insert_leaf(Node *leaf, vector<Node *> &parents, shared_ptr<char[]> key);
	
	void insert_nonleaf(Node *node, vector<Node *> &parents, int pos, splitReturn childsplit);
	
	int insert_binary(Node *cursor, const char* key, int low, int high, bool &equal);
	int search_binary(Node *cursor, const char* key, int low, int high);
	
	Node* search_leaf_node(Node *root, shared_ptr<char[]> key, vector<Node *> &parents,bool print=false);
	bool check_split_condition(Node *node);
	int split_point(vector<Key_c> allkeys);
	
	splitReturn split_nonleaf(Node *node, vector<Node *> parents, int pos, splitReturn childsplit);
	splitReturn split_leaf(Node *node, vector<Node *> &parents, shared_ptr<char[]> newkey);
	
	void bt_fetch_root(Node *currRoot, vector<Node *> &parents);
	
	Node* scan_node(Node *node, shared_ptr<char[]> key,bool print=false);
	Node* move_right(Node *node, shared_ptr<char[]> key);
#ifndef CHARALL
	vector<char*> decompress_keys(Node *node, int pos = -1);
#endif
};