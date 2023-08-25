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

class Data
{
public:
    // const char *ptr;
    uint8_t size; // Be careful;
    Data() : addr_(""), size(0){};
    Data(const char *str);
    Data(const char *p, int len);
    Data(const std::string &s);
    ~Data();
    Data(const Data &) = default;
    Data &operator=(const Data &) = default;
    const char *addr();
    // void set(const char *p, int s);
    // void from_string(string s);
private:
    char *addr_;
};

class Key
{
public:
    vector<string> ridList;
    string value;
    Key(string value, int rid);
    void addRecord(int rid);
    int getSize();
};

class Key_c
{
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
class Node
{
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

class Node
{
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
    Node(bool tail);
    ~Node();
};
#endif

class PrefixMetaData
{
public:
    int low;
    int high;
    string prefix;
    PrefixMetaData();
    PrefixMetaData(string p, int l, int h);
};

class DB2Node
{
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
class KeyMyISAM
{
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

class NodeMyISAM
{
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
class KeyWT
{
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

class NodeWT
{
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
class KeyPkB
{
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

class NodePkB
{
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

struct uncompressedKey
{ // for pkb
    string key;
    char *keyptr;
};

struct splitReturn
{
    string promotekey;
    Node *left;
    Node *right;
};

struct splitReturn_new
{
    Data *promotekey;
    Node *left;
    Node *right;
};

struct splitReturnDB2
{
    string promotekey;
    DB2Node *left;
    DB2Node *right;
};

struct splitReturnMyISAM
{
    string promotekey;
    NodeMyISAM *left;
    NodeMyISAM *right;
};

struct splitReturnWT
{
    string promotekey;
    NodeWT *left;
    NodeWT *right;
};

struct splitReturnPkB
{
    string promotekey;
    char *keyptr;
    NodePkB *left;
    NodePkB *right;
};

struct nodeBounds
{
    string lowerbound;
    string upperbound;
};

void printKeys(Node *node, bool compressed);
void printKeys_db2(DB2Node *node, bool compressed);
void printKeys_myisam(NodeMyISAM *node, bool compressed);
void printKeys_wt(NodeWT *node, bool compressed);
void printKeys_pkb(NodePkB *node, bool compressed);



#define NewPage() (char *)malloc(MAX_SIZE_IN_BYTES * sizeof(char))

#define UpdateBase(node, newbase) \
    {                             \
        delete node->base;        \
        node->base = newbase;     \
    }

#define UpdatePtrs(node, newptrs, num) \
    {                             \
        for(int i = 0; i < num; i++)\
            node->ptrs[i] = newptrs[i];\
        node->ptr_cnt = num;     \
    }

#define UpdateSize(node, newsize) \ 
{\
    delete node->keys_size; \
    node->keys_size = newsize; \
}

#define UpdateOffset(node, newoffset) \
    {                             \
        delete node->keys_offset;        \
        node->keys_offset = newoffset;     \
    }

#define GetKey(nptr, idx) (char *)(nptr->base + nptr->keys_offset[idx])

#define PageTail(nptr) nptr->base + nptr->memusage

#define InsertOffset(nptr, pos, offset)                      \
    {                                                        \
        for (int i = nptr->size; i > pos; i--)               \
            nptr->keys_offset[i] = nptr->keys_offset[i - 1]; \
        nptr->keys_offset[pos] = (uint16_t)offset;                     \
    }

#define InsertSize(nptr, pos, len)                       \
    {                                                    \
        for (int i = nptr->size; i > pos; i--)           \
            nptr->keys_size[i] = nptr->keys_size[i - 1]; \
        nptr->keys_size[pos] = (uint8_t)len;                      \
    }

// #define InsertNode(nptr, pos, newnode)         \
//     {                                          \
//         for (int i = nptr->size; i > pos; i--) \
//             nptr->ptrs[i] = nptr->ptrs[i - 1]; \
//         nptr->ptrs[pos] = newnode;             \
//         nptr->ptr_cnt += 1; \
//     }
// #define InsertSize(nptr, pos, len) \
//     nptr->keys_size.emplace(nptr->keys_size.begin() + pos, len)

#define InsertNode(nptr, pos, newnode) \
{\
    nptr->ptrs.emplace(nptr->ptrs.begin() + pos, newnode);\
    nptr->ptr_cnt += 1; \
}

// Insert k into nptr[pos]
#define InsertKey(nptr, pos, k, klen)            \
    {                                            \
        strcpy(PageTail(nptr), k);               \
        InsertOffset(nptr, pos, nptr->memusage); \
        InsertSize(nptr, pos, klen);\
        nptr->memusage += klen + 1;              \
        nptr->size += 1;                         \
    }

// Copy node->keys[low, high) to Page(base, mem, idx)

#define CopyKeyToPage(node, low, high, base, mem, idx, size) \
    for (int i = low; i < high; i++)                   \
    {                                                  \
        char *k = GetKey(node, i);                   \
        int klen = node->keys_size[i];              \
        strcpy(base + mem, k);                         \
        idx[i - (low)] = mem;                            \
        size[i - (low)] = klen;\
        mem += klen + 1;                          \
    }
