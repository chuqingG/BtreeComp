#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <string.h>
#include "../multithreading/rwlock.cpp"

using namespace std;

enum accessMode
{
	READ = 0,
	WRITE = 1,
};


int MAX_NODE_SIZE = 4;

string MAXHIGHKEY = "infinity";

// BP-std node
class Node
{
public:
	bool IS_LEAF;
	int id;
	vector<string> keys;
	int size;
	vector<Node *> ptrs;
	Node *next;		// Next node pointer
	string lowkey;  //Low key for each node
	string highkey; // High key for each node
	int level;		// Level of the node
	string prefix;  //Prefix of node in case of head compression
	ReadWriteLock rwLock;
	Node();
	~Node();
	void lock(accessMode mode);
	void unlock(accessMode mode);
};

class PrefixMetaData{
public:
	string prefix;
	int low;
	int high;
	PrefixMetaData();
	PrefixMetaData(string p, int l, int h);
};

//BP DB2 Node
class DB2Node{
public:      
    bool IS_LEAF;
	int id;
	vector<string> keys;
	int size;
	vector<DB2Node*> ptrs;
	DB2Node *next;		// Next node pointer
	string highkey; // High key for each node
	int level;		// Level of the node
	vector<PrefixMetaData> prefixMetadata;
	ReadWriteLock rwLock;
	DB2Node();
	~DB2Node();
	void lock(accessMode mode);
	void unlock(accessMode mode);
};

// Key with prefix and suffix encoding
class keyMyISAM{
public:
	string value;
	// Using a uchar equivalent from the MyISAM source code
	u_char *prefix;
	bool is1Byte;
	keyMyISAM(string value, int prefix);
	int getPrefix();
	void setPrefix(int prefix);
	int getSize();
};

//BP MyISAM Node
class NodeMyISAM{
public:
	bool IS_LEAF;
	int id;
	vector<keyMyISAM> keys;
	int size;
	vector<NodeMyISAM *> ptrs;
	NodeMyISAM *next;		// Next node pointer
	string highkey; // High key for each node
	int level;		// Level of the node
	ReadWriteLock rwLock;
	NodeMyISAM();
	~NodeMyISAM();
	void lock(accessMode mode);
	void unlock(accessMode mode);
};

class KeyWT{
public:
	string value;
	uint8_t prefix;
	bool isinitialized;
	string initialized_value;
	shared_mutex *mLock;     //mutex specific to updating key initialized value
	KeyWT(string value, uint8_t prefix);
	int getSize();
};

//BP WT Node
class NodeWT{
public:
	bool IS_LEAF;
	int id;
	vector<KeyWT> keys;
	int size;
	vector<NodeWT *> ptrs;
	uint32_t prefixstart; /* Best page prefix starting slot */
	uint32_t prefixstop;  /* Maximum slot to which the best page prefix applies */
	NodeWT *next;		// Next node pointer
	string highkey; 	// High key for each node
	int level;			// Level of the node
	ReadWriteLock rwLock;
	NodeWT();
	~NodeWT();
	void lock(accessMode mode);
	void unlock(accessMode mode);
};

const int PKB_LEN = 2;

class KeyPkB{
public:
	int16_t offset;
	char partialKey[PKB_LEN + 1];
	char *original;
	int pkLength;
	KeyPkB(int offset, string value, char *ptr);
	KeyPkB(string value);
	int getSize();
	void updateOffset(int offset);
};

class NodePkB{
public:
	bool IS_LEAF;
	int id;
	vector<KeyPkB> keys;
	int size;
	vector<NodePkB *> ptrs;
	NodePkB *next;		// Next node pointer
	string lowkey;  	//Low key for base key computation
	string highkey; 	// High key for each node
	int level;			// Level of the node
	ReadWriteLock rwLock;
	NodePkB();
	~NodePkB();
	void lock(accessMode mode);
	void unlock(accessMode mode);
};

struct uncompressedKey{  //for pkb
    string key;
    char *keyptr;
};

struct splitReturn{
    string promotekey;
    Node *left;
    Node *right;
};

struct splitReturnDB2{
    string promotekey;
    DB2Node *left;
    DB2Node *right;
};

struct splitReturnMyISAM{
    string promotekey;
    NodeMyISAM *left;
    NodeMyISAM *right;
};

struct splitReturnWT{
    string promotekey;
    NodeWT *left;
    NodeWT *right;
};

struct splitReturnPkB{
    string promotekey;
	char *keyptr;
    NodePkB *left;
    NodePkB *right;
};

struct nodeBounds{
    string lowerbound;
    string upperbound;
};

void printKeys(Node *node, bool compressed);
void printKeys_db2(DB2Node *node, bool compressed);
void printKeys_myisam(NodeMyISAM *node, bool compressed);
void printKeys_wt(NodeWT *node, bool compressed);
void printKeys_pkb(NodePkB *node, bool compressed);