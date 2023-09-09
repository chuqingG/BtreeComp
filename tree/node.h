#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <string.h>
#include <memory>
#include <cstring>
#include "../include/config.h"

using namespace std;

int MAX_NODE_SIZE = 4;

// Key represented as <key, {rid list}>
// str representation of rids for easy comparison and prefix compression
// if other approaches are used

class Data {
public:
    // const char *ptr;
    uint8_t size; // Be careful;
    Data() :
        addr_(""), size(0){};
    // Data(const char *str);
    Data(const char *p, int len);
    ~Data();
    // Data(const Data &);
    Data &operator=(const Data &);
    const char *addr();
    // void set(const char *p, int s);
    // void from_string(string s);
private:
    char *addr_;
};

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
const char MAXHIGHKEY[] = "infinity";

class Node {
public:
    bool IS_LEAF;
    int size;
    // vector<uint16_t> keys_offset;
    // vector<uint8_t> keys_size;
    vector<Node *> ptrs;
    uint16_t *keys_offset;
    uint8_t *keys_size;
    // Node **ptrs;
    uint8_t ptr_cnt;
    Data *lowkey;
    Data *highkey;
    Data *prefix;
    Node *prev; // Prev node pointer
    Node *next; // Next node pointer
    uint16_t memusage;
    char *base;
    Node(bool head, bool tail);
    ~Node();
};
#endif

#ifdef DUPKEY
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
#else
class PrefixMetaData {
public:
    int low;
    int high;
    Data *prefix;
    PrefixMetaData();
    PrefixMetaData(const char *str, int len, int l, int h);
    // PrefixMetaData(const PrefixMetaData &p);
    PrefixMetaData &operator=(const PrefixMetaData &);
    // ~PrefixMetaData();
};

class DB2Node {
public:
    bool IS_LEAF;
    // vector<Key_c> keys;
    char *base;
    int size;
    uint16_t *keys_offset;
    uint8_t *keys_size;
    int memusage;
    vector<DB2Node *> ptrs;
    uint8_t ptr_cnt;
    DB2Node *prev; // Prev node pointer
    DB2Node *next; // Next node pointer
    vector<PrefixMetaData> prefixMetadata;
    DB2Node();
    ~DB2Node();
};
#endif
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

#ifdef DUPKEY
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
#endif

// with no duplicate key
#ifdef WTCACHE
struct WThead {
    uint16_t key_offset;
    uint16_t initval_offset;
    uint8_t key_len;
    uint8_t init_len = 0;
    uint8_t pfx_len;
    bool initialized = false;
} __attribute__((packed));
#else
struct WThead {
    uint16_t key_offset;
    uint8_t key_len;
    uint8_t pfx_len;
} __attribute__((packed));
#endif

struct WTitem {
    char *addr;
    uint8_t size;
    bool newallocated = false;
    ~WTitem() {
        if (newallocated) {
            delete addr;
        }
    }
    WTitem &operator=(WTitem &old) {
        addr = old.addr;
        size = old.size;
        newallocated = false;
        return *this;
    }
};

#ifndef DUPKEY
class NodeWT {
public:
    bool IS_LEAF;
    int size; // Total key number
    char *base;
    uint16_t space_top;
    uint16_t space_bottom;

    uint16_t prefixstart; /* Best page prefix starting slot */
    uint16_t prefixstop;  /* Maximum slot to which the best page prefix applies */

    vector<NodeWT *> ptrs;
    NodeWT *prev; // Prev node pointer
    NodeWT *next; // Next node pointer
    uint16_t ptr_cnt;
    NodeWT();
    ~NodeWT();
};
#else
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
#endif

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

struct splitReturn_new {
    Data *promotekey;
    Node *left;
    Node *right;
};

struct splitReturnDB2 {
    Data *promotekey;
    DB2Node *left;
    DB2Node *right;
};

struct splitReturnMyISAM {
    string promotekey;
    NodeMyISAM *left;
    NodeMyISAM *right;
};

struct splitReturnWT {
    WTitem promotekey;
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
