#pragma once
#include "node.h"
#include "node_inline.h"

// Constructor of key
Key::Key(string val, int rid) {
    value = val;
    ridList.push_back(to_string(rid));
}

// Method to add new rid with same key value
void Key::addRecord(int rid) {
    ridList.push_back(to_string(rid));
}

// Method to get size of key
int Key::getSize() {
    int totalLen = value.length();
    for (int i = 0; i < ridList.size(); i++) {
        totalLen += ridList.at(i).length();
    }
    return totalLen;
}

Key_c::Key_c(char *val, int rid) {
    value = val;
    ridList.push_back(rid);
}

// Method to add new rid with same key value
void Key_c::addRecord(int rid) {
    ridList.push_back(rid);
}

// Method to get size of key
int Key_c::getSize() {
    int totalLen = strlen(value);
    totalLen += ridList.size() * sizeof(int);
    return totalLen;
}

void Key_c::update_value(string str) {
    delete value;
    char *val = new char[str.size() + 1];
    memcpy(val, str.data(), str.size() + 1);
    value = val;
}

// Constructor of Node
Node::Node() {
    size = 0;
    ptr_cnt = 0;
    space_top = 0;
    base = NewPage();
    SetEmptyPage(base);

    lowkey = new Item(true);
    highkey = new Item(); // this hk will be deallocated automatically
    highkey->addr = new char[9];
    strcpy(highkey->addr, MAXHIGHKEY);
    highkey->newallocated = true;
    prefix = new Item(true);

    prev = nullptr;
    next = nullptr;
}

// Destructor of Node
Node::~Node() {
    delete lowkey;
    delete highkey;
    delete prefix;
    delete base;
}

//===============Below for DB2===========

NodeDB2::NodeDB2() {
    size = 0;
    pfx_size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    pfxbase = new char[DB2_PFX_MAX_SIZE];
    memset(pfxbase, 0, sizeof(char) * DB2_PFX_MAX_SIZE);

    space_top = 0;
    pfx_top = 0;

    ptr_cnt = 0;
    InsertPfxDB2(this, 0, "", 0, 0, 0);
}

// Destructor of NodeDB2
NodeDB2::~NodeDB2() {
    delete pfxbase;
    delete base;
}

/*
==================For WiredTiger================
*/

NodeWT::NodeWT() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
#ifdef WTCACHE
    prefixstart = 0;
    prefixstop = 0;
#endif
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
}

// Destructor of NodeWT
NodeWT::~NodeWT() {
    // for (NodeWT *childptr : ptrs) {
    //     delete childptr;
    // }
    delete base;
}

NodeMyISAM::NodeMyISAM() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
}

// Destructor of NodeMyISAM
NodeMyISAM::~NodeMyISAM() {
    delete base;
}

NodePkB::NodePkB() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
}

// Destructor of NodePkB
NodePkB::~NodePkB() {
    delete base;
}

void printKeys(Node *node, bool compressed) {
    if (compressed && node->prefix->addr)
        cout << node->prefix->addr << ": ";
    for (uint32_t i = 0; i < node->size; i++) {
        Stdhead *head = GetHeaderStd(node, i);
        if (compressed && node->prefix->addr) {
            cout << PageOffset(node, head->key_offset) << ",";
        }
        else {
            cout << node->prefix->addr << PageOffset(node, head->key_offset) << ",";
        }
    }
}

void printKeys_db2(NodeDB2 *node, bool compressed) {
    for (uint32_t i = 0; i < node->pfx_size; i++) {
        DB2pfxhead *pfx = GetHeaderDB2pfx(node, i);
        if (compressed) {
            cout << "Prefix " << PfxOffset(node, pfx->pfx_offset) << ": ";
        }
        for (int l = pfx->low; l <= pfx->high; l++) {
            // Loop through rid list to print duplicates
            DB2head *head = GetHeaderDB2(node, l);
            if (compressed) {
                cout << PageOffset(node, head->key_offset) << ",";
            }
            else {
                cout << PfxOffset(node, pfx->pfx_offset) << PageOffset(node, head->key_offset) << ",";
            }
        }
    }
}

void printKeys_myisam(NodeMyISAM *node, bool compressed) {
    char *prev_key;
    char *curr_key;
    for (uint32_t i = 0; i < node->size; i++) {
        MyISAMhead *head = GetHeaderMyISAM(node, i);
        if (compressed || head->pfx_len == 0 || i == 0) {
            curr_key = PageOffset(node, head->key_offset);
            cout << unsigned(head->pfx_len) << ":" << curr_key << ",";
        }
        else {
            curr_key = new char[head->pfx_len + head->key_len + 1];
            strncpy(curr_key, prev_key, head->pfx_len);
            strcpy(curr_key + head->pfx_len, PageOffset(node, head->key_offset));
            cout << curr_key << ",";
        }
        prev_key = curr_key;
    }
}

void printKeys_wt(NodeWT *node, bool compressed) {
    char *prev_key;
    char *curr_key;
    for (int i = 0; i < node->size; i++) {
        WThead *head = GetHeaderWT(node, i);
        if (compressed || head->pfx_len == 0 || i == 0) {
            curr_key = PageOffset(node, head->key_offset);
            cout << unsigned(head->pfx_len) << ":" << curr_key << ",";
        }
        else {
            curr_key = new char[head->pfx_len + head->key_len + 1];
            strncpy(curr_key, prev_key, head->pfx_len);
            strcpy(curr_key + head->pfx_len, PageOffset(node, head->key_offset));
            cout << curr_key << ",";
        }

        prev_key = curr_key;
    }
}

void printKeys_pkb(NodePkB *node, bool compressed) {
    string curr_key;
    for (int i = 0; i < node->size; i++) {
        PkBhead *head = GetHeaderPkB(node, i);
        if (compressed) {
            cout << unsigned(head->pfx_len) << ":" << head->pk << "("
                 << PageOffset(node, head->key_offset) << ")"
                 << ",";
        }
        else {
            cout << PageOffset(node, head->key_offset) << ",";
        }
    }
}