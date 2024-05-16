#pragma once
#include "node.h"
#include "node_inline.h"

// Constructor of Node
Node::Node() {
    size = 0;
    ptr_cnt = 0;
    space_top = 0;
    base = NewPage();
    SetEmptyPage(base);

    lowkey = new Item(true);
    highkey = new Item(); // this hk will be deallocated automatically
    // highkey->addr = new char[9];
    // strcpy(highkey->addr, MAXHIGHKEY);
    // highkey->newallocated = true;
    highkey->size = 0;
    prefix = new Item(true);
    IS_LEAF = true;
    prev = nullptr;
    next = nullptr;

#ifdef UBS
    I = 0;
    Ip = 0;
    firstL = 0;
#endif
}

// Destructor of Node
Node::~Node() {
    delete lowkey;
    delete highkey;
    delete prefix;
    delete[] base;
}

int findk(Node *n, const char *key, int keylen) {
    // return the pos of the key, 0 for not found, size for this node too small
    int low = 0, high = n->size;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Stdhead *header = GetHeaderStd(n, mid);

        int cmp = char_cmp_new(key, PageOffset(n, header->key_offset),
                               keylen, header->key_len);
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return n->IS_LEAF ? -1 : high + 1;
}

void Node::erase(const char *k, int keylen, int idx = -1) {
    if (idx < 0) // uninitialized
        idx = findk(this, k, keylen);
    if (idx < 0)
        cout << "[Not found] The key doesn't exist in the tree" << endl;
    if (idx < this->size) {
        // Found the key in the current node
        if (IS_LEAF) {
            // Remove from the leaf
        }
        else {
        }
    }
}

//===============Below for DB2===========

NodeDB2::NodeDB2() {
    size = 0;
    pfx_size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPageDB2();
    SetEmptyPageDB2(base);
    IS_LEAF = true;
    space_top = 0;
    pfx_top = 0;

    ptr_cnt = 0;
    InsertPfxDB2(this, 0, "", 0, 0, 0);
}

// Destructor of NodeDB2
NodeDB2::~NodeDB2() {
    delete[] base;
}

/*
==================For WiredTiger================
*/

NodeWT::NodeWT() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;

    prefixstart = 0;
    prefixstop = 0;

    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodeWT
NodeWT::~NodeWT() {
    delete[] base;
}

NodeMyISAM::NodeMyISAM() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodeMyISAM
NodeMyISAM::~NodeMyISAM() {
    delete[] base;
}

NodePkB::NodePkB() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodePkB
NodePkB::~NodePkB() {
    delete[] base;
}

void printKeys(Node *node, bool compressed) {
#ifdef CHECK
    if(node->IS_LEAF) return;
#endif
    if (compressed && node->prefix->addr)
        cout << node->prefix->addr << ": ";
    for (int i = 0; i < node->size; i++) {
        Stdhead *head = GetHeaderStd(node, i);

        if (compressed && node->prefix->addr) {
            #ifdef KN 
                int l = head->key_len < PV_SIZE ? PV_SIZE : head->key_len;
                char *prefix = new char[l + 1]; prefix[l] = '\0';
                memcpy(prefix, head->key_prefix, PV_SIZE);
                if (PV_SIZE < l) strncpy(prefix + PV_SIZE, PageOffset(node, head->key_offset), head->key_len - PV_SIZE);
                char *conv = string_conv(prefix, l, 0);
                cout << conv << ",";
                delete[] prefix;
                delete[] conv;
            #elif defined PV 
            char prefix[PV_SIZE + 1] = {0};
            strncpy(prefix, head->key_prefix, PV_SIZE);
            cout << prefix;
            cout  << PageOffset(node, head->key_offset) << ",";
            #else
            cout  << PageOffset(node, head->key_offset) << ",";
            #endif
        }
        else {
            
            cout << node->prefix->addr << PageOffset(node, head->key_offset) << ",";
        }
    }
}

void printKeys_db2(NodeDB2 *node, bool compressed) {
    for (int i = 0; i < node->pfx_size; i++) {
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
    for (int i = 0; i < node->size; i++) {
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