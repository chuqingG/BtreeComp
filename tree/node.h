#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <string.h>
#include <memory>
#include "../include/config.h"

using namespace std;

int MAX_NODE_SIZE = 4;

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
#ifdef DUPKEY
class Node {
public:
    bool IS_LEAF;
    vector<Key_c> keys;
    int size;
    vector<Node *> ptrs;
    string prefix;
    Node *prev; // Prev node pointer
    Node *next; // Next node pointer
    Node();
    ~Node();
};
#else
class Node {
public:
    bool IS_LEAF;
    char *base;
    int size;
    vector<uint16_t> keys_offset;
    vector<Node *> ptrs;
    string prefix;
    Node *prev; // Prev node pointer
    Node *next; // Next node pointer
    uint16_t memusage;
    Node();
    ~Node();
};
#endif

class PrefixMetaData {
public:
    int low;
    int high;
    string prefix;
    PrefixMetaData();
    PrefixMetaData(string p, int l, int h);
};

class DB2Node {
public:
    bool IS_LEAF;
    vector<Key_c> keys;
    int size;
    vector<DB2Node *> ptrs;
    DB2Node *prev; // Prev node pointer
    DB2Node *next; // Next node pointer
    vector<PrefixMetaData> prefixMetadata;
    DB2Node();
    ~DB2Node();
};

// Key with prefix and suffix encoding
// Duplicates represented as <key, {rid list}>
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

class NodeMyISAM {
public:
    bool IS_LEAF;
    vector<KeyMyISAM> keys;
    int size;
    vector<NodeMyISAM *> ptrs;
    NodeMyISAM *prev; // Prev node pointer
    NodeMyISAM *next; // Next node pointer
    NodeMyISAM();
    ~NodeMyISAM();
};

// Duplicates represented as <key, {rid list}>
class KeyWT {
public:
    string value;
    vector<string> ridList;
    uint8_t prefix;
    bool isinitialized;
    string initialized_value;
    KeyWT(string value, uint8_t prefix, int rid);
    KeyWT(string value, uint8_t prefix, vector<string> ridList);
    void addRecord(int rid);
    int getSize();
};

class NodeWT {
public:
    bool IS_LEAF;
    vector<KeyWT> keys;
    int size;
    vector<NodeWT *> ptrs;
    uint32_t prefixstart; /* Best page prefix starting slot */
    uint32_t prefixstop;  /* Maximum slot to which the best page prefix applies */
    NodeWT *prev;         // Prev node pointer
    NodeWT *next;         // Next node pointer
    NodeWT();
    ~NodeWT();
};

const int PKB_LEN = 2;

// Duplicates represented as <key, {rid list}>
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
    vector<KeyPkB> keys;
    int size;
    vector<NodePkB *> ptrs;
    NodePkB *prev; // Prev node pointer
    NodePkB *next; // Next node pointer
    NodePkB();
    ~NodePkB();
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
    DB2Node *left;
    DB2Node *right;
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
void printKeys_db2(DB2Node *node, bool compressed);
void printKeys_myisam(NodeMyISAM *node, bool compressed);
void printKeys_wt(NodeWT *node, bool compressed);
void printKeys_pkb(NodePkB *node, bool compressed);

#define GetKey(nptr, idx) (char *)(nptr->base + nptr->keys_offset[idx])

#define PageTail(nptr) nptr->base + nptr->memusage

#define InsertOffset(nptr, pos, offset) \
    nptr->keys_offset.emplace(nptr->keys_offset.begin() + pos, offset)

#define InsertNode(nptr, pos, newnode) \
    nptr->ptrs.emplace(nptr->ptrs.begin() + pos, newnode)

#define NewPage() (char *)malloc(MAX_SIZE_IN_BYTES * sizeof(char))

#define UpdateBase(node, newbase) \
    {                             \
        delete node->base;        \
        node->base = newbase;     \
    }

// Insert k into nptr[pos]
#define InsertKey(nptr, pos, k)                  \
    {                                            \
        strcpy(PageTail(nptr), k);               \
        InsertOffset(nptr, pos, nptr->memusage); \
        nptr->memusage += strlen(k) + 1;         \
        nptr->size += 1;                         \
    }

// Copy node->keys[low, high) to Page(base, mem, idx)
#define CopyKeyToPage(node, low, high, base, mem, idx) \
    for (int i = low; i < high; i++) {                 \
        char *k = GetKey(node, i);                     \
        strcpy(base + mem, k);                         \
        idx.push_back(mem);                            \
        mem += strlen(k) + 1;                          \
    }
