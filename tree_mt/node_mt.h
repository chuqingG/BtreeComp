#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <string.h>
#include <memory>
#include "../multithreading/rwlock.cpp"
#include "data.h"
#include "../include/config.h"

using namespace std;

enum accessMode {
    READ = 0,
    WRITE = 1,
};

int MAX_NODE_SIZE = 4;

const char MAXHIGHKEY[] = "infinity";

// Key represented as <key, {rid list}>
// str representation of rids for easy comparison and prefix compression
// if other approaches are used
class Key {
public:
    vector<string> ridList;
    string value;
    Key(string value, int rid);
    void addRecord(int rid);
    int getSize();
};

class Key_c {
public:
    vector<int> ridList;
    char *value;
    Key_c(char *value, int rid);
    void addRecord(int rid);
    int getSize();
    void update_value(string s);
};

// BP-std node
class Node {
public:
    bool IS_LEAF;
    int id;
    vector<Key_c> keys;
    int size;
    vector<Node *> ptrs;
    Node *prev;    // Prev node pointer
    Node *next;    // Next node pointer
    char *lowkey;  // Low key for each node
    char *highkey; // High key for each node
    int level;     // Level of the node
    string prefix; // Prefix of node in case of head compression
    ReadWriteLock rwLock;
    Node();
    ~Node();
    void lock(accessMode mode);
    void unlock(accessMode mode);
};

class PrefixMetaData {
public:
    string prefix;
    int low;
    int high;
    PrefixMetaData();
    PrefixMetaData(string p, int l, int h);
};

// BP DB2 Node
class NodeDB2 {
public:
    bool IS_LEAF;
    int id;
    vector<Key> keys;
    int size;
    vector<NodeDB2 *> ptrs;
    NodeDB2 *prev;  // Prev node pointer
    NodeDB2 *next;  // Next node pointer
    string highkey; // High key for each node
    int level;      // Level of the node
    vector<PrefixMetaData> prefixMetadata;
    ReadWriteLock rwLock;
    NodeDB2();
    ~NodeDB2();
    void lock(accessMode mode);
    void unlock(accessMode mode);
};

// Key with prefix and suffix encoding
class KeyMyISAM {
public:
    string value;
    vector<string> ridList;
    // Using a uchar equivalent from the MyISAM source code
    u_char *prefix;
    bool is1Byte;
    KeyMyISAM(string value, int prefix, int rid);
    KeyMyISAM(string value, int prefix, vector<string> ridList);
    int getPrefix();
    void setPrefix(int prefix);
    void addRecord(int rid);
    int getSize();
};

// BP MyISAM Node
class NodeMyISAM {
public:
    bool IS_LEAF;
    int id;
    vector<KeyMyISAM> keys;
    int size;
    vector<NodeMyISAM *> ptrs;
    NodeMyISAM *prev; // Prev node pointer
    NodeMyISAM *next; // Next node pointer
    string highkey;   // High key for each node
    int level;        // Level of the node
    ReadWriteLock rwLock;
    NodeMyISAM();
    ~NodeMyISAM();
    void lock(accessMode mode);
    void unlock(accessMode mode);
};

class KeyWT {
public:
    string value;
    vector<string> ridList;
    uint8_t prefix;
    bool isinitialized;
    string initialized_value;
    shared_mutex *mLock; // mutex specific to updating key initialized value
    KeyWT(string value, uint8_t prefix, int rid);
    KeyWT(string value, uint8_t prefix, vector<string> ridList);
    void addRecord(int rid);
    int getSize();
};

// BP WT Node
class NodeWT {
public:
    bool IS_LEAF;
    int id;
    vector<KeyWT> keys;
    int size;
    vector<NodeWT *> ptrs;
    uint32_t prefixstart; /* Best page prefix starting slot */
    uint32_t prefixstop;  /* Maximum slot to which the best page prefix applies */
    NodeWT *prev;         // Prev node pointer
    NodeWT *next;         // Next node pointer
    string highkey;       // High key for each node
    int level;            // Level of the node
    ReadWriteLock rwLock;
    NodeWT();
    ~NodeWT();
    void lock(accessMode mode);
    void unlock(accessMode mode);
};

const int PKB_LEN = 2;

class KeyPkB {
public:
    int16_t offset;
    char partialKey[PKB_LEN + 1];
    char *original;
    int pkLength;
    vector<string> ridList;
    KeyPkB(int offset, string value, char *ptr, int rid);
    KeyPkB(int offset, string value, char *ptr, vector<string> ridList);
    void addRecord(int rid);
    int getSize();
    void updateOffset(int offset);
};

class NodePkB {
public:
    bool IS_LEAF;
    int id;
    vector<KeyPkB> keys;
    int size;
    vector<NodePkB *> ptrs;
    NodePkB *prev;  // Prev node pointer
    NodePkB *next;  // Next node pointer
    string lowkey;  // Low key for base key computation
    string highkey; // High key for each node
    int level;      // Level of the node
    ReadWriteLock rwLock;
    NodePkB();
    ~NodePkB();
    void lock(accessMode mode);
    void unlock(accessMode mode);
};

struct uncompressedKey { // for pkb
    string key;
    char *keyptr;
};

struct splitReturn {
    string promotekey;
    Node *left;
    Node *right;
};

struct splitReturnDB2 {
    string promotekey;
    NodeDB2 *left;
    NodeDB2 *right;
};

struct splitReturnMyISAM {
    string promotekey;
    NodeMyISAM *left;
    NodeMyISAM *right;
};

struct splitReturnWT {
    string promotekey;
    NodeWT *left;
    NodeWT *right;
};

struct splitReturnPkB {
    string promotekey;
    char *keyptr;
    NodePkB *left;
    NodePkB *right;
};

struct nodeBounds {
    string lowerbound;
    string upperbound;
};

void printKeys(Node *node, bool compressed);
void printKeys_db2(NodeDB2 *node, bool compressed);
void printKeys_myisam(NodeMyISAM *node, bool compressed);
void printKeys_wt(NodeWT *node, bool compressed);
void printKeys_pkb(NodePkB *node, bool compressed);